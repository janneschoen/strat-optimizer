# Strat‑Optimizer

**High‑performance grid‑search backtesting engine for systematic trading strategies.**

Grid‑search a strategy's parameter space over historical price data, rank
combinations by risk‑adjusted return, validate the winner on unseen data,
and visualise the parameter‑to‑performance surface.

---

I had a trading strategy I wanted to test, so I wrote a backtester for it.
Then I realised: why am I guessing the parameters?  The backtester was
already there — I just needed to wrap it in a loop and let the machine find
what worked.  That grew into grid search, then out‑of‑sample validation,
then equity curves and heatmaps.  The simulation loop ended up in C because
a few thousand combinations over years of daily data would have taken hours
otherwise.

---

## Table of Contents

1. [Design](#design)
2. [Features](#features)
3. [Metrics](#metrics)
4. [Installation](#installation)
5. [Quick Start](#quick-start)
6. [Configuration Reference](#configuration-reference)
7. [Pipeline Walkthrough](#pipeline-walkthrough)
8. [Writing a Strategy](#writing-a-strategy)
9. [Visualisation Gallery](#visualisation-gallery)
10. [Performance](#performance)
11. [Project Structure](#project-structure)

---

## Design

### Two‑language architecture

| Layer          | Language | Role                                                    |
|----------------|----------|---------------------------------------------------------|
| Orchestrator   | Python   | Config parsing, data download, grid generation, plotting |
| Backtest engine | C        | Simulation loop, signal dispatch, metric computation     |

The Python side never touches trading logic; the C side never touches JSON.
They communicate exclusively through plain‑text temp files in `.temp/` —
no sockets, no pipes, no serialisation formats.  This keeps the two
processes fully decoupled and independently debuggable.

### IPC model

```
config.json  →  Python  →  .temp/parameters.temp     →  C engine
                           .temp/prices.temp          →  C engine
                           ←  .temp/performances.temp
                           ←  .temp/equity.temp
```

### Parallelism

When evaluating multiple parameter combinations, the C engine parallelises
with OpenMP (`schedule(dynamic)`).  Each thread declares its own equity
curve on the stack — there are zero shared mutable buffers, so no locks
are needed outside the progress counter.

---

## Features

- **Grid search** over arbitrary parameter ranges with configurable step sizes
- **Constraint‑aware filtering** — invalid combinations (e.g. Fast SMA ≥ Slow SMA) are pruned before simulation
- **Walk‑forward validation** — train/test split with the test window held out until after selection
- **Two performance metrics** per combination (both annualised for comparability across horizons)
- **Equity curve generation** for single‑combination runs
- **Visualisations for 1–3 free parameters:** scatter + linear fit, 2‑D heatmap, 3‑D scatter
- **Extensible strategy framework** — add a C signal function + a JSON metadata entry

---

## Metrics

Let $P_0, P_1, \dots, P_T$ be the portfolio value at each time step, with
$T$ trading days simulated and $Y$ trading days per year (252 for
equities, 365 for crypto/forex).

### Annualised Profit (CAGR)

$$
\text{CAGR} = \left(\frac{P_T}{P_0}\right)^{Y/T} - 1
$$

### Annualised Sharpe Ratio

Let $r_t = \frac{P_t - P_{t-1}}{P_{t-1}}$ be the daily return at time $t$.
With risk‑free rate $R_f = 0$:

$$
\text{Sharpe} = \frac{\bar{r}}{\sigma_r} \cdot \sqrt{Y}
$$

where $\bar{r}$ is the sample mean of daily returns and $\sigma_r$ is
the population standard deviation.

Both metrics are computed in C and written to disk as `annual_profit,
sharpe_ratio` per combination.

---

## Installation

```bash
# 1. Clone the repository
git clone https://github.com/janneschoen/strat-optimizer.git
cd strat-optimizer

# 2. Build the C engine (requires GCC with OpenMP)
make

# 3. Create and activate a virtual environment
python -m venv .venv
source .venv/bin/activate          # Linux / macOS
# .venv\Scripts\activate           # Windows

# 4. Install Python dependencies
pip install -r requirements.txt
```

**Dependencies:**
- C compiler with OpenMP support (GCC or Clang)
- Python ≥ 3.8
- Core Python packages: `numpy`, `matplotlib`, `yfinance`

---

## Quick Start

### 1. Create `config.json`

```json
{
    "strategy_name": "SMA Crossover",
    "strategies_file": "strategies.json",

    "parameter_ranges": [[1, 200], [30, 200], [0.2, 0.2]],
    "parameter_steps":  [3, 3, 0],
    "backtest_length": 2000,
    "test_size": 0.4,

    "asset": {
        "ticker": "BTC-USD",
        "is_traded_all_year": true
    }
}
```

### 2. Run

```bash
python main.py                    # uses config.json
python main.py my_config.json     # custom config file
```

### 3. Output

The program prints:

```
Best parameters:
  Fast SMA Length: 1.0
  Slow SMA Length: 120.0
  Position Sizing: 0.2

            Sharpe Ratio  Annual Profit
Training           2.1431         0.3840
Testing            1.8712         0.3127
```

Followed by interactive Matplotlib figures:
- Heatmap of Sharpe ratio / annual profit vs parameters (training)
- Equity curve of the best combination (test window)

---

## Configuration Reference

### `config.json`

| Key | Type | Description |
|-----|------|-------------|
| `strategy_name` | string | Name matching an entry in `strategies.json` |
| `strategies_file` | string | Path to the strategy definitions file |
| `parameter_ranges` | `[[low, high], ...]` | One `[low, high]` pair per strategy parameter |
| `parameter_steps` | `[int, ...]` | Grid step size per parameter; `0` = fixed at `range[0]` |
| `backtest_length` | int | Number of **trading days** to test (from yesterday backwards) |
| `test_size` | float | Fraction of data held out for walk‑forward testing `[0, 1]` |
| `asset.ticker` | string | Yahoo Finance ticker (e.g. `BTC-USD`, `AAPL`) |
| `asset.is_traded_all_year` | bool | `true` for crypto/forex (365 d/y), `false` for equities (252 d/y) |

### `strategies.json`

A JSON array of strategy definitions.  Each entry:

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | Strategy identifier (matched against `config.strategy_name`) |
| `parameters` | `[{...}]` | List of parameter descriptors |

Each parameter descriptor:

| Key | Type | Required | Description |
|-----|------|----------|-------------|
| `name` | string | yes | Display name (appears on plot axes) |
| `min` | float | no | Lower bound (inclusive) |
| `max` | float | no | Upper bound (inclusive) |
| `upper_param` | int | no | Index of a parameter this value must be **strictly less than** |
| `defines_lookback` | bool | no | Exactly one parameter per strategy must have this; its maximum value determines how many historical prices the signal function receives |

**Example** (`strategies.json`):

```json
[
    {
        "name": "SMA Crossover",
        "parameters": [
            {"name": "Fast SMA Length", "min": 1, "upper_param": 1},
            {"name": "Slow SMA Length", "min": 1, "defines_lookback": true},
            {"name": "Position Sizing", "min": 0, "max": 1}
        ]
    },
    {
        "name": "RSI",
        "parameters": [
            {"name": "Buying Threshold",  "min": 0, "max": 100, "upper_param": 1},
            {"name": "Selling Threshold", "min": 0, "max": 100},
            {"name": "Window Size",       "min": 1, "defines_lookback": true}
        ]
    }
]
```

---

## Pipeline Walkthrough

```
config.json
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 1. Load & validate config                               │
│    • Match strategy name to strategies.json entry        │
│    • Derive lookback from defines_lookback parameter     │
│    • Set trading_days = 252 or 365                      │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 2. Download prices  (prices.py → yfinance)              │
│    • Fetch (backtest_length + lookback) days of data     │
│    • Write as newline‑delimited floats to .temp/         │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 3. Generate parameter grid  (parameters.py)              │
│    • Cartesian product of per‑parameter ranges           │
│    • Filter through strategy.is_valid() constraints      │
│    • Write combinations to .temp/parameters.temp         │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 4. Train  (C engine, first 1−test_size of data)          │
│    • Spawn ./compute with key:value CLI args             │
│    • Each combination → backtest() → performance_t       │
│    • Single combo: save equity curve to .temp/           │
│    • Multiple combos: OpenMP parallel, equity discarded  │
│    • Read back performances from .temp/                  │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 5. Select best combination                               │
│    • Maximise Sharpe ratio over the training window      │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 6. Test  (C engine, walk‑forward on last test_size)      │
│    • Run best combination on held‑out data               │
│    • Save equity curve for plotting                      │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 7. Visualise                                            │
│    • Heatmap / scatter of param → performance (train)    │
│    • Equity curve with linear trend (test)               │
└─────────────────────────────────────────────────────────┘
```

---

## Writing a Strategy

Adding a new strategy requires two steps: a C signal function and a JSON
metadata entry.

### 1. Signal Function (C)

Create a new `.c` file (e.g. `03-MyStrategy.c`) containing a function with
this signature:

```c
#include "common.h"

float signal_MyStrategy(unsigned            day,
                        strategy_config_t * strat,
                        float             * prices)
{
    // Access parameters:
    //   strat->params[0], strat->params[1], ...
    //
    // Access scratch storage (persists across days for this run):
    //   strat->storage[0]   (up to STRAT_STORAGE slots)
    //   Reset to NAN by the engine between runs.
    //
    // Access historical prices:
    //   prices[day], prices[day - 1], ...  (raw, no lookahead guard)

    // Return exposure ∈ [−1.0, +1.0]:
    //   +1 = 100 % long    0 = flat    −1 = 100 % short

    return 0.0f;
}
```

**Contract:**

| Aspect | Rule |
|--------|------|
| **Return type** | `float ∈ [−1.0, 1.0]` — fraction of net worth to allocate. Positive = long, negative = short. |
| **Price data** | `prices[i]` for `i ≤ day` is real; `prices[i]` for `i > day` is garbage.  Do **not** peek forward. |
| **State** | Use `strat->storage[]` for day‑to‑day state.  The engine zeroes it between combinations with NAN. |
| **Cost model** | No transaction costs, no slippage.  Rebalancing is frictionless. |
| **Wipeout** | If net worth drops to ≤ 0, the engine stops simulating and zeros the remainder of the equity curve. |

### 2. Register the Function

Declare the function in `common.h`:

```c
float signal_MyStrategy(unsigned day,
                        strategy_config_t * strat,
                        float * prices);
```

Add it to the dispatch table in `backtesting.c`:

```c
float (*get_signal[])(unsigned day,
                      strategy_config_t * strategy_config,
                      float * prices) = {
    signal_SMA_crossover,
    signal_RSI,
    signal_MyStrategy,      // ← append here
};
```

The index in this array must match the strategy's position in
`strategies.json`.

### 3. Define Properties (JSON)

Add an entry to `strategies.json`:

```json
{
    "name": "My Strategy",
    "parameters": [
        {"name": "Param A", "min": 1, "upper_param": 1},
        {"name": "Param B", "min": 1, "defines_lookback": true},
        {"name": "Param C", "min": 0, "max": 1}
    ]
}
```

**Constraints you can express:**

| Constraint | JSON key | Example |
|------------|----------|---------|
| Lower bound | `"min"` | Param A ≥ 1 |
| Upper bound | `"max"` | Param C ≤ 1 |
| Must be less than another param | `"upper_param"` | Param A < Param B |
| Defines required lookback | `"defines_lookback"` | The engine provides at least `max(Param B)` historical prices |

Only `min`, `max`, and `upper_param` constraints are enforced during grid
generation.  Any additional validation logic belongs in the C signal function.

### 4. Build & Run

```bash
make              # recompile with the new .c file
python main.py    # run with a config that references "My Strategy"
```

---

## Visualisation Gallery

The plotting module auto‑detects the number of **free** parameters (those with
`parameter_steps[i] ≠ 0`) and chooses the appropriate visualisation.

### 1 free parameter → 2‑D scatter + linear fit
<img width="931" height="472" alt="Fig1" src="https://github.com/user-attachments/assets/ae525f26-43ce-4fbc-86f8-bffc9f8b922e" />

Each point is one backtest.  The blue line is an ordinary least‑squares linear
fit.  Here, Buying Threshold of the RSI strategy is tested from 30 to 60 (Selling Threshold fixed at a higher value),
and a positive correlation between annual profit and the parameter is observed within the test.

### 2 free parameters → 2‑D heatmap
<img width="931" height="472" alt="Fig2" src="https://github.com/user-attachments/assets/f1d63c1f-e8da-4975-b083-45bc5f0f1502" />

Colour encodes the performance metric.  The triangle shape reflects the
`upper_param` constraint (Fast SMA < Slow SMA).  Brighter points near the
bottom edge indicate short fast‑SMA windows performed best in this window.

### 3 free parameters → 3‑D scatter
<img width="931" height="472" alt="Fig3" src="https://github.com/user-attachments/assets/dced04f6-cc1d-4132-9385-cbd9ccb49663" />

Each point is one combination; colour encodes the performance metric (here
annual profit for the SMA Crossover strategy on BTC‑USD).  Rotate and zoom in Matplotlib
for a better angle.

### Equity curve (single‑combination or test run)
<img width="931" height="472" alt="Fig_Equity" src="https://github.com/user-attachments/assets/f4130ad6-6e92-418b-8260-3ad8cdeaeaec" />

Green = portfolio value over time (normalised to start at 1.0).  Black =
linear regression trend line.  Shown here is the test‑phase walk‑forward run
on the best parameter combination found during training — performance on data
the optimisation never saw.

---

## Performance

Throughput is strategy‑dependent — a simple SMA crossover evaluates faster
than an RSI with a 200‑day window.  Representative numbers (consumer laptop,
~1340 events/s on `sysbench cpu run`):

| Scenario | Combinations | Backtest days | Total days simulated | Wall time | Throughput |
|----------|-------------|---------------|---------------------|-----------|------------|
| SMA Crossover, BTC‑USD | 2,166 | 2,000 | 4.3 M | ~20 s | ~217k days/s |

The C engine is the bottleneck; the Python layer contributes negligible
overhead (< 1 % of runtime).  OpenMP scaling is near‑linear on machines with
≤ 8 physical cores for typical grid sizes.

### What affects simulation speed

- **Lookback length** — longer windows mean more SMA/RSI computation per day
- **Strategy complexity** — simple arithmetic vs iterative indicators
- **Grid density** — doubling the number of combinations roughly doubles runtime
- **Price series length** — linear in the number of days simulated

---

## Project Structure

```
strat-optimizer/
├── main.py                  # Entry point — orchestrates the full pipeline
├── config.py                # JSON → RunConfig dataclass
├── parameters.py            # Cartesian product grid with constraint filtering
├── prices.py                # Yahoo Finance downloader
├── backtesting.py           # Python ↔ C bridge (subprocess + temp files)
├── strategies.py            # Strategy metadata & parameter validation
├── plotting.py              # 2‑D/3‑D visualisation dispatch
├── equity_curve.py          # Equity curve plot with linear trend
│
├── common.h                 # Shared C types: performance_t, strategy_config_t, run_config_t
├── config.c                 # CLI key:value parser (populates run_config_t)
├── backtesting.c            # Core simulation loop + signal dispatch table
├── core.c                   # Main driver: load data, run backtests, write results
├── 01-SMA-Crossover.c       # Strategy: Simple Moving Average crossover
├── 02-RSI.c                 # Strategy: Relative Strength Index
│
├── strategies.json          # Strategy definitions (names, parameter constraints)
├── example.json             # Example run configuration
├── config.json              # Active run configuration (user‑provided)
├── requirements.txt         # Python dependencies
├── Makefile                 # C build (GCC + OpenMP)
│
└── .temp/                   # IPC temp files (auto‑created, gitignored)
    ├── prices.temp
    ├── parameters.temp
    ├── performances.temp
    └── equity.temp
```

---

## Ideas

- **Transaction costs.** A fixed fee or basis‑point spread per trade would
  penalise high‑turnover strategies and bring backtest results closer to
  reality.  Simpler to add than it sounds — just deduct from `cash` on
  each rebalance in `backtesting.c`.
- **Multi‑asset.** Run the same strategy (or different ones) across several
  tickers and combine the equity curves into a portfolio.  Most of the
  plumbing is already there — the C engine just needs a loop over assets.
