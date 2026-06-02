from config import RunConfig
from dataclasses import dataclass
import subprocess
import numpy as np
import json

C_PROGRAM_NAME = "compute"

# Indexes used in C
ANNUAL_PROFIT = 0
SHARPE_RATIO = 1

@dataclass
class Performance:
    sharpe_ratio: float
    annual_profit: float

def run_backtesting_engine(run: RunConfig, number_of_prices: int, combinations: List[Tuple[float]], test_mode: bool = False) -> List[Performance]:
    
    # Write parameter combinations to file

    with open(run.parameter_path, 'w') as file:
        for combo in combinations:
            for param in combo:
                file.write(str(param)+" ")
            file.write("\n")


    # Calculate start and end days

    simulatable_days = number_of_prices - run.lookback
    training_days = int(simulatable_days * (1.0 - run.test_size))

    if test_mode:
        start = run.lookback + training_days
        end = number_of_prices
    else:
        start = run.lookback
        end = run.lookback + training_days
    

    # Prepare data

    backtest_data = {
        "start": start,
        "end": end,

        "number_of_prices": number_of_prices,
        "number_of_combinations": len(combinations),
        "number_of_parameters": run.strategy.number_of_parameters,

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

    subprocess.run(arguments)


    # Read performances from file

    try:
        raw_performances = np.loadtxt(run.performances_path, delimiter=',').tolist()
    except:
        raise RuntimeError("Got no performances from backtesting engine.")
    
    if test_mode:
        return [
            Performance(
                annual_profit = raw_performances[ANNUAL_PROFIT],
                sharpe_ratio = raw_performances[SHARPE_RATIO]
            )
        ]

    performances = []
    for performance in raw_performances:
        performances.append(
            Performance(
                annual_profit = performance[ANNUAL_PROFIT],
                sharpe_ratio = performance[SHARPE_RATIO]
            )
        )

    return performances