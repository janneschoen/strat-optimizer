"""
main.py — Entry point for a strat-optimizer run

Pipeline:
  1. Load the JSON config      → RunConfig
  2. Download historical prices → .temp/prices.temp
  3. Generate a grid of parameter combinations (Cartesian product,
     filtered by strategy constraints)
  4. Spawn the C backtest engine on the TRAINING period
  5. Select the best combination by maximum Sharpe ratio
  6. Re-run the C engine on the TEST period (walk-forward validation)
  7. Plot:
       - heatmap / scatter of performance vs parameters
       - equity curve of the best combination on the test set
"""

from .prices import download_prices
from .plotting import plot
from .equity_curve import show_equity_curve
from .strategies import Strategy
from .config import load_config
from .parameters import generate_parameter_combinations
from .backtesting import run_backtesting_engine

def main():

    run = load_config()

    number_of_prices = download_prices(run)

    number_of_combinations, parameter_combos = \
        generate_parameter_combinations(run)

    # ---- in-sample: train on the first (1 − test_size) of the data ----
    performances = run_backtesting_engine(
        run,
        number_of_prices,
        combinations=parameter_combos
    )

    # select the combination with the highest Sharpe ratio
    best_performance = performances[0]
    best_combination  = parameter_combos[0]

    for p in range(number_of_combinations):
        perf = performances[p]
        if perf.sharpe_ratio > best_performance.sharpe_ratio:
            best_performance = perf
            best_combination  = parameter_combos[p]

    # ---- out-of-sample: test on the remaining (test_size) of the data ----
    test_performance = run_backtesting_engine(
        run,
        number_of_prices,
        combinations=[best_combination],
        test_mode=True
    )[0]

    print("Best parameters:\n")
    for p in range(run.strategy.number_of_parameters):
        print(f"  {run.strategy.parameters[p].name}: {best_combination[p]}")

    print(f"\n{'':10} {'Sharpe Ratio':>15} {'Annual Profit':>15}")
    print(f"{'Training':10} {best_performance.sharpe_ratio:>15.4f} "
          f"{best_performance.annual_profit:>15.4f}")
    print(f"{'Testing':10}  {test_performance.sharpe_ratio:>15.4f} "
          f"{test_performance.annual_profit:>15.4f}")

    # ---- visualise ----
    annual_profits = [p.annual_profit for p in performances]
    sharpe_ratios  = [p.sharpe_ratio  for p in performances]

    plot(run, annual_profits, parameter_combos, metric="Annual Profit")
    plot(run, sharpe_ratios,  parameter_combos, metric="Sharpe Ratio")

    show_equity_curve(run)


if __name__ == "__main__":
    main()
