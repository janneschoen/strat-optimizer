# Strat-Optimizer
This project performs large-scale backtesting of a trading strategy by grid-searching strategy parameters.
Core computations and simulation logic are implemented in C for performance, while the input handling and visualisation of results is in Python.
The main purpose is to see which parameters of a trading strategy have performed best historically.


## Features
- High-performance backtest engine in C
- Visualisation of effect parameters have on performance
- Grid search over configurable parameter ranges
- Generating equity curves for single parameter combinations

## Requirements
- C compiler (gcc/clang)
- Python 3.8+ (libraries: yfinance, matplotlib)
- pip

## Installation
```bash
git clone https://github.com/janneschoen/strat-optimizer.git
cd strat-optimizer
make
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```
## Usage
By default, the program looks for a `config.json` file that contains the settings to run.
See `example.json` for an example configuration.
```bash
# With 'config.json':
python main.py
# With custom .json file:
python main.py myConfig.json
```
## Example Usage
### Strategy: Simple Moving Average Crossover
In this example, we try to find out which parameters of the "SMA Crossover" strategy worked best in the last 2000 days when applied to Bitcoin-USD.
The trading logic of this strategy type is defined in `0_SMA_crossover.c`, and the parameter names and boundaries in `main.py`.
It works like this:
- Each day, two moving averages are calculated - "Slow" and "Fast"
  - "Slow SMA Length" is 20 -> calculate average price of last 20 days
  - "Fast SMA Length" is 5 -> calculate average price of last 5 days
- Fast SMA crosses ____ Slow SMA:
    - above -> interpreted as upwards momentum, so a long position is entered
    - below -> interpreted as downwards momentum, so a short position is entered
- Size of position depends on the third parameter, which we fixed to 20% in this example
### Configuration
The relevant part of the config file used in this example:
```json
{
    "stratType": 0,
    "params": [[1, 200], [30, 200], [0.2]],
    "paramSteps": [3, 3, 0],

    "backtestLength": 2000,
    "ticker": "BTC-USD",
    "fullYear": 1,
}
```
- `"stratType": 0`: Index of the strategy type - defined in array `strategyTypes` in `main.py` and array of `getSignal()` functions in `backtesting.c`
- `"params": [[1, 200], ...]`: Testing range of parameters, in format [from, to]
- `"paramSteps": [3, 3, 0]`: Steps when iterating through each parameter: 3 -> [1, 4, 7, ...],  0 -> fixed parameter

- `"backtestingLength": 2000`: Length of backtest in trading days up to yesterday
- `"ticker": "BTC-USD"`: Ticker symbol of asset on finance.yahoo.com
- `"fullYear": 1`: If the asset trades all year or is affected by weekends and holidays (0: stocks, ETFs | 1: crypto, forex)

### Results
Date of test: March 23 2026
The two parameters tested in this example are the length of the "Fast Moving Average" and the "Slow Moving Average".
Plotting these two parameters in relationship to the annualized profit results in a 2-dimensional heatmap.
In this test, 2166 combinations were tested, and the computation took around ~20 seconds.

<img width="931" height="472" alt="testResults" src="https://github.com/user-attachments/assets/88e2b5d2-0484-4e72-82e1-1eb9848ff62d" />
Visualisation of results is triangle-shaped, because by definition of the SMA Crossover strategy,
the "Fast SMA Length" has to be shorter than the "Slow SMA Length".

#### Interpretation

It can clearly be observed that shorter "Fast SMA" lengths tended to perform better in the backtest,
and the most profitable combinations seem to be where the Fast SMA Length is close to 1.
It looks like the "sweet spot" has its center at (1, 120), and extends vertically.

### Single Test / Equity Curve
In the next step, a test will be run on a specific pair of parameters, generating an equity curve.
To do so, enter the desired parameters as the first part of the ranges, and set the steps to 0.
```json
{
    "params": [[1, 200], [120, 200], [0.2]],
    "paramSteps": [0, 0, 0],
}
```
The following output is generated:
```bash
Annualized Profit: 0.193
```
<img width="931" height="472" alt="equityCurve" src="https://github.com/user-attachments/assets/20857a62-1f15-42d9-8c8f-1a86e2952fce" />
The green graph shows the portfolio value over time, starting at 1, and the black line is the linear regression line.

#### Interpretation

The graph indicates high returns, and in an almost linear fashion - but when backtesting,
it has to be kept in mind that this specific strategy is optimized to the timeframe we are testing it on.
At the start of the 2000 days, we could not have known that these parameters would bring such results.
Past performance is not indicative of future results.
