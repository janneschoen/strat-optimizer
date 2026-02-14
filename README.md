
# Strategy Optimizer

The program is backtesting trading strategies with different parameters, and tries to give a recommendation about which parameters to use in future trading decisions.

## How to use
There are two execution modes: range testing and running a single test.

When testing a range, the program will backtest all parameter combinations in the given ranges, and plot how parameters and strategy performance interact.

A single test gives the user more information about the development of the portfolio value during the backtest.

### Range testing
Expected arguments:

1. 0 (for range mode)
2. Visualisations (0 / 1)
3. Strategy Type ID
4. Upper limit of parameter testing range (10 -> 1-10 will be tested)
5. Length of backtest
6. Asset ticker
7. Full year? (365 or 252 days) (1 / 0)

Example range test:
./stratOpt 0 0 0 50 50 5 500 PLTR 0

Example single test:
./stratOpt 1 0 0 29 40 3 500 BTC-USD 1
