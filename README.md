
# Strategy Optimizer

The program is backtesting trading strategies with different parameters, and tries to give a recommendation about which parameters to use in future trading decisions.

## How to use
There are two execution modes: range testing and running a single test.

When testing a range, the program will backtest all parameter combinations in the given ranges, and plot how parameters and strategy performance interact.

A single test gives the user more information about the development of the portfolio value during the backtest.

### Expected Arguments

In order:

- ID of strategy type
- strategy parameters (or upper range limits)
- backtesting length
- asset ticker
- 252 or 365 days (0 / 1)

- range or single test (0 / 1)
- goal: annual profit or sharpe (0 / 1)
- visualisation (0 / 1)

### Example use cases

./stratOpt 0 50 50 10 500 PLTR 0 0 1 1
This will backtest the strategy type 0 on Palantir stock.
Each backtest will be 500 time units, and weekends / holidays won't be counted.
All parameter combinations up to [50, 50, 10] will be tested.
The strategy will be optimized for sharpe ratio, and the results will be visualised.

Example single test:
./stratOpt 0 23 41 4 600 BTC-USD 1 1 0 0
This will backtest the strategytype 0 on Bitcoin-USD.
The backtest will be 600 time units, with trades all year.
Only the single parameter combination [23, 41, 4] will be tested.
The annual profit will be looked at, and the backtest will not be visualised.
