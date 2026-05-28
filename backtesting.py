from config import RunConfig
import subprocess
import numpy as np
import json

C_PROGRAM_NAME = "compute"

def run_backtesting_engine(run: RunConfig, number_of_prices: int, number_of_combinations: int) -> List[float]:

    backtest_data = {
        "number_of_prices": number_of_prices,
        "number_of_combinations": number_of_combinations,
        "number_of_parameters": run.strategy.number_of_parameters,

        "lookback": run.lookback,
        "strategy_index": run.strategy.index,
        "trading_days": run.asset.trading_days,

        "prices_path": run.prices_path,
        "equity_path": run.equity_path,
        "parameter_path": run.parameter_path,
        "performances_path": run.performances_path
    }

    # Pass data as pseudo-json-object to C engine

    arguments = [f"./{C_PROGRAM_NAME}"]

    for key, value in backtest_data.items():
        arguments.append(f"{key}:{value}")

    subprocess.run(arg for arg in arguments)

    # Read profits from file

    try:
        performances = np.loadtxt(run.performances_path, delimiter=',').tolist()
    except:
        raise RuntimeError("Got no performances from backtesting engine.")

    return performances