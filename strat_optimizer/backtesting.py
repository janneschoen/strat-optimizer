"""
backtesting.py — Python ↔ C bridge for the backtest engine

Calls directly into libengine.so via ctypes.  All data flows through
numpy arrays in shared memory — no subprocess, no temp files, no
serialisation overhead.
"""

from .config import RunConfig
from dataclasses import dataclass
from pathlib import Path
import ctypes
import numpy as np

# ---- load the shared library ---------------------------------------
_LIB_PATH = Path(__file__).parent.parent / "libengine.so"
_lib = ctypes.CDLL(str(_LIB_PATH))


class EngineArgs(ctypes.Structure):
    _fields_ = [
        ("prices",         ctypes.POINTER(ctypes.c_float)),
        ("param_grid",     ctypes.POINTER(ctypes.c_float)),
        ("performances",   ctypes.POINTER(ctypes.c_float)),
        ("equity_curve",   ctypes.POINTER(ctypes.c_float)),
        ("n_prices",       ctypes.c_uint),
        ("n_combos",       ctypes.c_uint),
        ("n_params",       ctypes.c_uint),
        ("strategy_index", ctypes.c_uint),
        ("start",          ctypes.c_uint),
        ("end",            ctypes.c_uint),
        ("trading_days",   ctypes.c_uint),
    ]

_lib.engine_run.argtypes = [ctypes.POINTER(EngineArgs)]
_lib.engine_run.restype  = None

# ---- data types ----------------------------------------------------

@dataclass
class Performance:
    sharpe_ratio:  float
    annual_profit: float


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

    # ---- build the flat parameter grid -----------------------------
    param_grid = np.array(combinations, dtype=np.float32).ravel()

    # ---- allocate output buffers -----------------------------------
    performances_out = np.zeros(n_combos * 2, dtype=np.float32)
    equity_out       = np.zeros(n_days,    dtype=np.float32)

    # ---- populate the args struct ----------------------------------
    args = EngineArgs()
    args.prices         = prices.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.param_grid     = param_grid.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.performances   = performances_out.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.equity_curve   = equity_out.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.n_prices       = number_of_prices
    args.n_combos       = n_combos
    args.n_params       = n_params
    args.strategy_index = run.strategy.index
    args.start          = start
    args.end            = end
    args.trading_days   = run.asset.trading_days

    # ---- call the C engine -----------------------------------------
    _lib.engine_run(ctypes.byref(args))

    # ---- unpack performances ---------------------------------------
    perfs = []
    for i in range(n_combos):
        perfs.append(Performance(
            annual_profit = float(performances_out[i * 2]),
            sharpe_ratio  = float(performances_out[i * 2 + 1]),
        ))

    return perfs, equity_out
