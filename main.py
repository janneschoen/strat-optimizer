import json
from prices import download_prices
from plotting import plot
from equity_curve import show_equity_curve
from strategies import Strategy
import subprocess
import itertools, numpy as np
import os, sys

def main():
    args = sys.argv
    if len(args) == 1:
        config_file = 'config.json'
    else:
        config_file = args[1]

    print(f"Reading configuration from '{config_file}'")

    with open(config_file, 'r') as file:
        config = json.load(file)

    temp_dir = config["temp_directory"] + "/"

    os.makedirs(temp_dir, exist_ok=True)
    parameter_path = temp_dir + config["parameter_file"]
    performance_path = temp_dir + config["results_file"]
    equity_path = temp_dir + config["equity_file"]
    prices_path = temp_dir + config["prices_file"]

    parameter_ranges = config["parameter_ranges"]
    strategy_name = config["strategy"]
    backtest_length = config["backtest_length"]
    parameter_steps = config["parameter_steps"]
    is_traded_all_year = config["is_traded_all_year"]


    with open(config["strategy_file"], 'r') as file:
        strategies = json.load(file)
    
    strategy_names = [strategy["name"] for strategy in strategies]

    if strategy_name not in strategy_names:
        raise KeyError(f"Strategy '{strategy_name}' not defined.")

    
    for strat in strategies:
        if strat["name"] == strategy_name:
            strategy = Strategy(
                strat["name"],
                strat["parameters"]
            )
            break

    print("Strategy type:", strategy.name)


    # Downloading price data

    for i, parameter in enumerate(strategy.parameters):
        if "defines_lookback" in parameter:
            lookback = parameter_ranges[i][1]

    number_of_prices = backtest_length + lookback
    
    download_prices(number_of_prices, config)


    # Generating parameter combinations if steps provided

    parameter_combos = []

    if len(parameter_steps) != len(parameter_ranges):
        raise ValueError(f"Got {len(parameter_ranges)} parameters but {len(parameter_steps)} intervals.")

    parameter_lists = []
    for parameter_range in parameter_ranges:
        step = parameter_steps[len(parameter_lists)]
        if step == 0:
            parameter_lists.append([parameter_range[0]])
        else:
            parameter_lists.append(np.arange(parameter_range[0], parameter_range[1], step))

    all_combos = list(itertools.product(*parameter_lists))

    # Only save parameter combos that pass validity test

    for combo in all_combos:
        if strategy.is_valid(combo):
            parameter_combos.append(combo)

    number_of_combinations = len(parameter_combos)

    if(number_of_combinations) == 0:
        raise ValueError("Could not generate any valid parameter combinations.")
    
    print("Number of parameter combinations:", number_of_combinations)


    # Saving parameter combos in file

    with open(parameter_path, 'w') as file:
        for combo in parameter_combos:
            for param in combo:
                file.write(str(param)+" ")
            file.write("\n")


    # Running the backtesting engine

    arguments = [
        number_of_prices,
        prices_path,
        number_of_combinations,
        parameter_path,
        strategy.number_of_parameters,
        strategy,
        lookback,
        performance_path,
        equity_path
    ]

    subprocess.run(["./compute"] + [str(argument) for argument in arguments])


    # Annualizing profits

    try:
        performances = np.loadtxt(performance_path).tolist()
    except:
        raise RuntimeError("Got no performances from backtesting engine.")


    year_length = 365 if is_traded_all_year else 252

    
    if number_of_combinations > 1:
        for p in range(number_of_combinations):
            performances[p] = (1 + performances[p]) ** (year_length / backtest_length) - 1
    else:
        performances = (1 + performances) ** (year_length / backtest_length) - 1


    # Plotting results

    if number_of_combinations > 1:
        plot(parameter_steps, strategy, parameter_path, performances, config)
    else:
        print(f"Annualized profit: {performances:.3f}")
        show_equity_curve(equity_path, strategy, config)


if __name__ == "__main__":
    main()
