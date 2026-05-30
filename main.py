from prices import download_prices
from plotting import plot
from equity_curve import show_equity_curve
from strategies import Strategy
from config import load_config
from parameters import generate_parameter_combinations
from backtesting import run_backtesting_engine

def main():

    # Read json data

    run = load_config()

    # Download price data

    number_of_prices = download_prices(run)

    # Generate parameter combinations

    number_of_combinations, parameter_combos = generate_parameter_combinations(run)


    # Run the backtesting engine

    performances = run_backtesting_engine(
        run,
        number_of_prices,
        combinations = parameter_combos
    )

    best_performance = performances[0]
    best_combination = None

    for p in range(number_of_combinations):
        performance = performances[p]
        if performance.sharpe_ratio > best_performance.sharpe_ratio:
            best_performance = performance
            best_combination = parameter_combos[p]


    # Test best sharpe combination out of sample
    
    test_performance = run_backtesting_engine(
        run,
        number_of_prices,
        combinations = [best_combination],
        test_mode = True
    )[0]

    print("Best parameters:\n")
    for p in range(run.strategy.number_of_parameters):
        print(f"{run.strategy.parameters[p].name}: {best_combination[p]}")

    print(f"\n{'':10} {'Sharpe Ratio':>15} {'Annual Profit':>15}")
    print(f"{'Training':10} {best_performance.sharpe_ratio:>15.4f} {best_performance.annual_profit:>15.4f}")
    print(f"{'Testing':10} {test_performance.sharpe_ratio:>15.4f} {test_performance.annual_profit:>15.4f}")


    # Plot results / show equity curve

    if number_of_combinations > 1:

        annual_profits = [p.annual_profit for p in performances]
        sharpe_ratios = [p.sharpe_ratio for p in performances]

        plot(run, annual_profits, parameter_combos, metric = "Annual Profit")
        plot(run, sharpe_ratios, parameter_combos, metric = "Sharpe Ratio")
    else:
    
        print(f"Annualized profit: {performances.annual_profit:.3f}")
        print(f"Sharpe ratio: {performance.sharpe_ratio:.3f}")
    
        show_equity_curve(run)


if __name__ == "__main__":
    main()
