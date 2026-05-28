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

Follow these steps to set up the project locally:

```bash
# 1. Clone the repository
git clone https://github.com/janneschoen/strat-optimizer.git
cd strat-optimizer

# 2. Build the C engine
make

# 3. Create a virtual environment (isolates dependencies)
python -m venv .venv

# 4. Activate the virtual environment
source .venv/bin/activate # Windows: .venv\Scripts\activate

# 5. Install the required Python packages
pip install -r requirements.txt
```

## Usage
By default, the program looks for a `config.json` file that contains the settings to run.
See `example.json` for an example configuration.

With 'config.json':
```bash
python main.py
```
With custom .json file:
```bash
python main.py customFile.json
```

## Example Usage
### Strategy: Simple Moving Average Crossover
In this example, we try to find out which parameters of the "SMA Crossover" strategy worked best in the last 2000 days when applied to Bitcoin-USD.

The trading logic of this strategy type is defined in `01-SMA_crossover.c`, and the parameter names and boundaries in `strategies.json`.

Long and short positions are entered based on the following criteria:
- Each day: calculate two moving averages: "Slow" and "Fast" (e.g. 20d, 5d)
- When these two price averages cross, a long or short position is entered
  - long when 5-day-average > 20-day-average, short when opposite
- Size of position depends on the third parameter, which is fixed to 20% in this example

### Configuration
The config file used in this example:
```json
{
    "strategy_name": "SMA Crossover",
    "strategies_file": "strategies.json",

    "parameter_ranges": [[1,200], [30,200], [0.2, 0.2]],
    "parameter_steps": [3, 3, 0],
    "backtest_length": 2000,

    "asset":{
        "ticker": "BTC-USD",
        "is_traded_all_year": true
    }
}
```
- `"strategy_name": "SMA Crossover`: Name of the trading strategy as defined in the 'strategies.json' file.
- `"parameter_ranges": [[1, 200], ...]`: Testing range of parameters, in format [from, to]
- `"parameter_steps": [3, 3, 0]`: Steps when iterating through each parameter
    - 3 -> [1, 4, 7, ...]
    - 0 -> fixed parameter

- `"backtesting_length": 2000`: Length of backtest in trading days up to yesterday
- `"ticker": "BTC-USD"`: Ticker symbol of asset on finance.yahoo.com
- `"is_traded_all_year": true`: If the asset trades 365 days a year
    - false: stocks, ETFs
    - true: crypto, forex

### Results (March 23, 2026)
The two parameters tested in this example are the length of the "Fast Moving Average" and the "Slow Moving Average".
Plotting these two parameters in relationship to the annualized profit results in a 2-dimensional heatmap.

<img width="700" height="472" alt="testResults" src="https://github.com/user-attachments/assets/88e2b5d2-0484-4e72-82e1-1eb9848ff62d" />

*Visualisation of results is triangle-shaped, because by definition of the SMA Crossover strategy,
the "Fast SMA Length" has to be shorter than the "Slow SMA Length".*

#### Interpretation

It can clearly be observed that shorter "Fast SMA" lengths tended to perform better in the backtest,
and the most profitable combinations seem to be where the Fast SMA Length is close to 1.
It looks like the "sweet spot" has its center at (1, 120), and extends vertically.

#### Computational Performance
In this test, 2.166 combinations were tested, with a backtest length of 2.000 days.
This means a total of 4.332.000 trading days were simulated.

The device the computation took place on scored ~1340 events per second during test ```sysbench cpu run```, indicating low to medium CPU performance.

Computations took ~20 seconds, resulting in a performance of ~216.600 simulated trading days per second.
Keep in mind that execution speed strongly depends on the type and parameters of the tested strategy.

### Single Test / Equity Curve
In the next step, a test will be run on a specific pair of parameters, generating an equity curve.
To do so, enter the desired parameters as the first part of the ranges, and set the steps to 0.
```json
{
    "parameter_ranges": [[1, 200], [120, 200], [0.2]],
    "parameter_steps": [0, 0, 0],
}
```
The following output is generated:
```bash
Annualized Profit: 0.193
```
<img width="700" height="472" alt="equityCurve" src="https://github.com/user-attachments/assets/20857a62-1f15-42d9-8c8f-1a86e2952fce" />

*The green graph shows the portfolio value over time, starting at 1, and the black line is the linear regression line.*

#### Interpretation

The graph indicates high returns, and in an almost linear fashion - but it has to be kept in mind that this specific strategy is optimized to the timeframe we are testing it on.
Moving average lengths can easily be overfit: while those specific lenghts would have brought a lot of profit in the observed time span, it is unknown whether they remain profitable parameters beyond it.

## Types of Visualisations
This is how results might look with different amounts of parameters.

### Two-dimensional - 1 parameter
<img width="600" alt="2d scatter plot" src="https://github.com/user-attachments/assets/2fb1a16b-14a9-41e3-a821-a711606deb40" />

A simple scatter plot with a regression line: backtest(parameter) = performance.

### Three-dimensional - 2 parameters
<img width="600" alt="2d heatmap" src="https://github.com/user-attachments/assets/2f5f9084-16dd-43fb-b3ef-1cf45773722f" />

A 2D heatmap to display the effect of 2 parameters on performance.

### Four-dimensional - 3 parameters
<img width="600" alt="fig3" src="https://github.com/user-attachments/assets/c55eff4f-2b14-4cbd-849f-bd097f51471b" />

The relationship between 3 parameters and strategy performance is visualised by a 3D heatmap.

## Your Own Strategies
Adding and testing your own strategies includes two steps: programming the function that generates the trading signals in *C*, and defining the strategies properties in *json*.

### 1. Signal
This is the core of the strategy - it decides the size and direction of exposure to the asset for each timestamp.

#### 1.1 Programming
It is implemented in a new C file.

Make sure it includes the header file:

```C
#include "common.h"
```
This is the structure of the function that generates trading signals:

```C
float myNewSignal(unsigned day, strat_t * strategy, float * prices);
```
It returns a float ∈ [-1.0, 1.0], which represents the desired asset exposure relative to ones networth: from fully short (-1) to fully long (1).

The parameters are accessible from the `strat_t` struct:
```C
float p0 = strategy->params[0];
```

#### 1.2 Inclusion
To let the program use the new signal function, make sure to declare it in `common.h`.

Also append it to this list of functions in `backtesting.c`:

```C
float (*getSignal[])(unsigned day, strat_t * strategy, float * prices) = {
    signal_1,
    signa_2,
    signal_3
};
```

### 2. Defining Properties
This part is done in the JSON file that contains the strategy properties - 'strategies.json' by default. It is necessary for validating parameters and displaying names.
```json
[
    {
        "name": "My Strategy",
        "parameters": [
            {"name": "Parameter A", "min": 1, "max": 50},
            {"name": "Parameter B", "min": 1, "defines_lookback": true},
            {"name": "Parameter C", "upper_param": 0}
        ]
    }
]
```
The JSON file contains a list of JSON objects, where each object is one strategy. You have to define the name of the strategy, as well as the properties of the parameters.

If a parameter is not allowed to be below or above a certain numeric limit, define it via 'min' or 'max'. In the example, the value of 'Parameter A' can not be smaller than 1 or bigger than 50.

When the value of a parameter must always be smaller than the value of another, 'upper_param' can be set to the index of the parameter that acts as an upper bound. In the example, 'Parameter C' can not numerically exceed 'Parameter A'.

One parameter has to define the necessary lookback period. In the example, 'Parameter B' defines the lookback, meaning it dictates the amount of past price data the program grants the signal generator function. If the value is 20, each time the function is called, it receives the last 20 prices of the asset.


### 3. Done
You should now be able to define parameter ranges in the `config.json` file and test your strategy as you like.
For visualisations, the program currently only supports testing 1-3 parameters at a time.
