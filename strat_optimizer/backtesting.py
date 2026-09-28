"""
backtesting.py — Python ↔ C bridge for the backtest engine

Calls directly into libengine.so via ctypes.  All data flows through
numpy arrays in shared memory — no subprocess, no temp files, no
serialisation overhead.
"""

from .config import RunConfig
from .strategy import MAX_PARAMS
from dataclasses import dataclass
from pathlib import Path
import ctypes
import numpy as np

# ---- load the shared library ---------------------------------------
_LIB_PATH = Path(__file__).parent.parent / "libengine.so"
try:
    _lib = ctypes.CDLL(str(_LIB_PATH))
except OSError as e:
    raise RuntimeError(
        f"Could not load the C engine at '{_LIB_PATH}'. "
        f"Build it first with `make`. ({e})"
    )


class EngineArgs(ctypes.Structure):
    # NOTE: this must mirror engine_args_t in src/engine.h exactly.
    # The size check below catches layout drift; field order must
    # still be kept in sync by hand.
    _fields_ = [
        ("prices",           ctypes.POINTER(ctypes.c_float)),
        ("param_grid",       ctypes.POINTER(ctypes.c_float)),
        ("performances",     ctypes.POINTER(ctypes.c_float)),
        ("equity_curve",     ctypes.POINTER(ctypes.c_float)),
        ("n_prices",         ctypes.c_uint),
        ("n_combos",         ctypes.c_uint),
        ("n_params",         ctypes.c_uint),
        ("strategy_name",    ctypes.c_char_p),
        ("start",            ctypes.c_uint),
        ("end",              ctypes.c_uint),
        ("trading_days",     ctypes.c_uint),
        ("transaction_cost", ctypes.c_float),
    ]

_lib.engine_run.argtypes = [ctypes.POINTER(EngineArgs)]
_lib.engine_run.restype  = None
_lib.engine_args_sizeof.restype = ctypes.c_size_t
_lib.engine_num_metrics.restype = ctypes.c_uint
_lib.engine_max_params.restype  = ctypes.c_uint

# ---- ABI / contract checks -----------------------------------------
# A stale libengine.so would otherwise be an invisible source of wrong
# results, so fail loudly at import time.
_expected_size = ctypes.sizeof(EngineArgs)
_engine_size   = _lib.engine_args_sizeof()
if _expected_size != _engine_size:
    raise RuntimeError(
        f"EngineArgs layout mismatch: ctypes={_expected_size} bytes, "
        f"C engine={_engine_size} bytes. Rebuild with `make`."
    )

# The single source of truth for the metric stride is the C engine.
NUM_METRICS = int(_lib.engine_num_metrics())
if NUM_METRICS != 7:
    raise RuntimeError(
        f"Expected 7 performance metrics per combination, "
        f"but the engine reports {NUM_METRICS}. Update Performance below."
    )

if int(_lib.engine_max_params()) != MAX_PARAMS:
    raise RuntimeError(
        "MAX_PARAMS mismatch between strat_optimizer/strategy.py "
        "and src/common.h."
    )


@dataclass
class Performance:
    annual_profit: float
    sharpe_ratio:  float
    total_return:  float
    max_drawdown:  float
    sortino_ratio: float
    calmar_ratio:  float
    volatility:    float


# ---- public API ----------------------------------------------------

def run_backtesting_engine(
    run:              RunConfig,
    number_of_prices: int,
    prices:           np.ndarray,
    combinations:     list,
    test_mode:        bool = False,
):
    """
    Run the C backtesting engine on the given parameter combinations.

    When test_mode=False (training):
        Uses the first (1 − test_size) of the available trading days.
        Returns (list_of_Performance, equity_curve_or_None).

    When test_mode=True (testing / walk-forward):
        Uses the last (test_size) of the available trading days.
        Returns (list_of_Performance, equity_curve).
    """

    # ---- compute the training / test window boundaries -------------
    simulatable_days = number_of_prices - run.lookback
    training_days    = int(simulatable_days * (1.0 - run.test_size))

    if test_mode:
        start = run.lookback + training_days
        end   = number_of_prices
    else:
        start = run.lookback
        end   = run.lookback + training_days

    n_combos = len(combinations)
    n_params = run.strategy.number_of_parameters
    n_days   = end - start

    if n_params > MAX_PARAMS:
        raise ValueError(
            f"Strategy '{run.strategy.name}' has {n_params} parameters, "
            f"but the engine supports at most {MAX_PARAMS}."
        )

    # ---- build the flat parameter grid -----------------------------
    param_grid = np.array(combinations, dtype=np.float32).ravel()

    # ---- allocate output buffers -----------------------------------
    performances_out = np.zeros(n_combos * NUM_METRICS, dtype=np.float32)
    equity_out       = np.zeros(n_days, dtype=np.float32)

    # Keep the encoded name alive for the duration of the call: ctypes
    # stores only the pointer in the struct.
    strategy_name = run.strategy.name.encode("utf-8")

    # ---- populate the args struct ----------------------------------
    args = EngineArgs()
    args.prices         = prices.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.param_grid     = param_grid.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.performances   = performances_out.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.equity_curve   = equity_out.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.n_prices       = number_of_prices
    args.n_combos       = n_combos
    args.n_params       = n_params
    args.strategy_name  = strategy_name
    args.start          = start
    args.end            = end
    args.trading_days     = run.asset.trading_days
    args.transaction_cost = run.transaction_cost

    # ---- call the C engine -----------------------------------------
    _lib.engine_run(ctypes.byref(args))

    # ---- unpack performances ---------------------------------------
    perfs = []
    for i in range(n_combos):
        base = i * NUM_METRICS
        perfs.append(Performance(
            annual_profit = float(performances_out[base + 0]),
            sharpe_ratio  = float(performances_out[base + 1]),
            total_return  = float(performances_out[base + 2]),
            max_drawdown  = float(performances_out[base + 3]),
            sortino_ratio = float(performances_out[base + 4]),
            calmar_ratio  = float(performances_out[base + 5]),
            volatility    = float(performances_out[base + 6]),
        ))

    return perfs, equity_out
