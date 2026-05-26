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

        "prices_path": run.prices_path,
        "equity_path": run.equity_path,
        "parameter_path": run.parameter_path,
        "performances_path": run.performances_path
    }

    # Passing data as pseudo-json-object to C engine

    arguments = [f"./{C_PROGRAM_NAME}"]

    for pair in backtest_data.items():
        arguments.append(f"{pair[0]}:{pair[1]}")

    subprocess.run(arg for arg in arguments)

    exit()


    # Annualizing profits

    try:
        performances = np.loadtxt(run.performances_path).tolist()
    except:
        raise RuntimeError("Got no performances from backtesting engine.")


    year_length = 365 if run.asset.is_traded_all_year else 252

    
    if number_of_combinations > 1:
        for p in range(number_of_combinations):
            performances[p] = (1 + performances[p]) ** (year_length / run.backtest_length) - 1
    else:
        performances = (1 + performances) ** (year_length / run.backtest_length) - 1
    
    return performances