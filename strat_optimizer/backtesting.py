"""
backtesting.py — Python ↔ C bridge for the backtest engine

Writes parameter combinations and prices to temp files, spawns the
compiled C binary (./compute) as a subprocess, then reads the
resulting performance metrics back.

The C binary is a plain CLI program that accepts key:value arguments:

    ./compute start:300 end:2100 number_of_prices:3300 ...

Communication is entirely file-based (no sockets, no pipes) so
the two processes are fully decoupled and debuggable independently.
"""

from .config import RunConfig
from dataclasses import dataclass
import subprocess
import numpy as np

C_PROGRAM_NAME = "compute"

# Column indices matching the C engine's output order
ANNUAL_PROFIT = 0
SHARPE_RATIO  = 1


@dataclass
class Performance:
    sharpe_ratio:  float
    annual_profit: float


def run_backtesting_engine(
    run:              RunConfig,
    number_of_prices: int,
    combinations:     list,
    test_mode:        bool = False,
):
    """
    Run the C backtesting engine on the given parameter combinations.

    When test_mode=False (training):
        Uses the first (1 − test_size) of the available trading days.
        Returns one Performance per combination.

    When test_mode=True (testing / walk-forward):
        Uses the last (test_size) of the available trading days.
        Returns a single-element list.
    """

    # ---- write parameter combinations to temp file ----------------
    with open(run.parameter_path, 'w') as f:
        for combo in combinations:
            f.write(' '.join(str(p) for p in combo) + '\n')

    # ---- compute the training / test window boundaries ------------
    simulatable_days = number_of_prices - run.lookback
    training_days    = int(simulatable_days * (1.0 - run.test_size))

    if test_mode:
        start = run.lookback + training_days   # test window start
        end   = number_of_prices
    else:
        start = run.lookback                   # first tradable day
        end   = run.lookback + training_days

    # ---- assemble CLI arguments for the C engine ------------------
    cli_data = {
        "start":                    start,
        "end":                      end,
        "number_of_prices":         number_of_prices,
        "number_of_combinations":   len(combinations),
        "number_of_parameters":     run.strategy.number_of_parameters,
        "strategy_index":           run.strategy.index,
        "trading_days":             run.asset.trading_days,
        "prices_path":              run.prices_path,
        "equity_path":              run.equity_path,
        "parameter_path":           run.parameter_path,
        "performances_path":        run.performances_path,
    }

    argv = [f"./{C_PROGRAM_NAME}"]
    for key, value in cli_data.items():
        argv.append(f"{key}:{value}")

    subprocess.run(argv)

    # ---- read the C engine's output -------------------------------
    try:
        raw = np.loadtxt(run.performances_path, delimiter=',')
    except Exception:
        raise RuntimeError(
            "Got no performances from backtesting engine. "
            "The C binary may have crashed."
        )

    # np.loadtxt returns 1-D for a single row, 2-D for multiple rows
    if raw.ndim == 1:
        raw = [raw.tolist()]
    else:
        raw = raw.tolist()

    if test_mode:
        return [Performance(
            annual_profit = raw[0][ANNUAL_PROFIT],
            sharpe_ratio  = raw[0][SHARPE_RATIO],
        )]

    return [
        Performance(
            annual_profit = row[ANNUAL_PROFIT],
            sharpe_ratio  = row[SHARPE_RATIO],
        )
        for row in raw
    ]
