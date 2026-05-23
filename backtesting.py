from config import RunConfig
import subprocess
import numpy as np

def run_backtesting_engine(run: RunConfig, number_of_prices: int, number_of_combinations: int) -> List[float]:

    # Starting C subprocess
    
    arguments = [
        number_of_prices,
        run.prices_path,
        number_of_combinations,
        run.parameter_path,
        run.strategy.number_of_parameters,
        run.strategy.index,
        run.lookback,
        run.performances_path,
        run.equity_path
    ]

    subprocess.run(["./compute"] + [str(argument) for argument in arguments])


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