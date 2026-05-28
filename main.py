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

    if number_of_combinations > 1:
        plot(run, performances, parameter_combos)
    else:
        print(f"Annualized profit: {performances:.3f}")
        show_equity_curve(run)


if __name__ == "__main__":
    main()
