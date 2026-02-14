
# Strat-Optimizer

The program is backtesting trading strategies with different parameters, and tries to give a recommendation about which parameters to use in future trading decisions.

## How to use
There are two execution modes: range testing and running a single test.

When testing a range, the program will backtest all parameter combinations in the given ranges, and plot how parameters and strategy performance interact.

A single test gives the user more information about the development of the portfolio value during the backtest.

### Range testing
Expected arguments:

1. 0 (for standard mode)
2. Strategy Type ID
3. Upper limit of parameter testing range (10 -> 1-10 will be tested)
4. Length of backtest
5. Asset ticker

Example use case:
./quant5k 0 0 50 50 5 500 PLTR

### Single test
Expected Arguments:
1. 1 (for testing mode)
2. Strategy Type ID
3. Parameters
4. Length of backtest
5. Asset ticker
Arguments: main, testMode, stratTypeID, p0, p1, p2, btLength, ticker

Example use case:
./quant5k 1 0 29 40 1 500 PLTR
