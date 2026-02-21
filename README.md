
# Strategy Optimizer

The program is backtesting trading strategies with different parameters, and tries to give a recommendation about which parameters to use in future trading decisions.

## How to use
There are two execution modes: range testing and running a single test.

When testing a range, the program will backtest all parameter combinations in the given ranges, and plot how parameters and strategy performance interact.

A single test gives the user more information about the development of the portfolio value during the backtest.

## Expected config
The program expects a json file "execMode.json" that contains the required arguments.
