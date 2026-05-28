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

    performances = run_backtesting_engine(run, number_of_prices, number_of_combinations)

    # Plot results / show equity curve

    ANNUAL_PROFIT = 0
    SHARPE_RATIO = 1

    if number_of_combinations > 1:

        annual_profits = [p[ANNUAL_PROFIT] for p in performances]
        sharpe_ratios = [p[SHARPE_RATIO] for p in performances]

        plot(run, annual_profits, parameter_combos, metric = "Annual Profit")
        plot(run, sharpe_ratios, parameter_combos, metric = "Sharpe Ratio")
    else:
    
        print(f"Annualized profit: {performances[ANNUAL_PROFIT]:.3f}")
        print(f"Sharpe ratio: {performances[SHARPE_RATIO]:.3f}")
    
        show_equity_curve(run)


if __name__ == "__main__":
    main()
