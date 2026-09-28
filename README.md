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
| Backtest engine | C        | Simulation loop, signal dispatch, metric computation (loaded as shared library via ctypes) |

The Python side never touches trading logic; the C side never touches JSON.
They communicate through a **shared library** — the C engine is compiled as
`libengine.so`, loaded directly into the Python process via `ctypes`, and
called as a regular function.  All data flows through numpy arrays in
shared memory — no subprocess, no temp files, no serialisation overhead.

### IPC model

```
config.json  →  Python  →  numpy float32 arrays
                                │
             ctypes.CDLL(        ▼
               "libengine.so") →  engine_run(engine_args_t *)
                                    │
                                    │  OpenMP-parallel backtest() loop
                                    │
                                    ▼
                           ←  performances[]  (annual_profit, sharpe_ratio)
                           ←  equity_curve[]  (single-combo only)

No files, no subprocess — raw pointers into the same memory.
```

### Parallelism

When evaluating multiple parameter combinations, the C engine parallelises
with OpenMP (`schedule(dynamic)`).  Each thread allocates its own equity
buffer on the heap (not the stack, so long backtests are safe) — there
are zero shared mutable buffers, so no locks are needed outside the
progress counter.

---

## Features

- **Grid search** over arbitrary parameter ranges with configurable step sizes
- **Constraint‑aware filtering** — invalid combinations (e.g. Fast SMA ≥ Slow SMA) are pruned before simulation
- **Walk‑forward validation** — train/test split with the test window held out until after selection
- **Seven performance metrics** per combination (all annualised for comparability across horizons)
- **Equity curve generation** for single‑combination runs
- **Visualisations for 1–3 free parameters:** scatter + linear fit, 2‑D heatmap, 3‑D scatter
- **Extensible strategy framework** — add a C signal function, register it by name, and add a JSON metadata entry; dispatch can never be broken by reordering

---

## Metrics

Let $P_0, P_1, \dots, P_T$ be the portfolio value at each time step, with
$T$ trading days simulated and $Y$ trading days per year (252 for
equities, 365 for crypto/forex).  Let
$r_t = \frac{P_t - P_{t-1}}{P_{t-1}}$ be the daily return at time $t$.
Risk‑free rate $R_f = 0$ throughout.

### Annualised Profit (CAGR)

$$
\text{CAGR} = \left(\frac{P_T}{P_0}\right)^{Y/T} - 1
$$

### Total Return

$$
\text{Total Return} = \frac{P_T - P_0}{P_0}
$$

### Annualised Sharpe Ratio

$$
\text{Sharpe} = \frac{\bar{r}}{\sigma_r} \cdot \sqrt{Y}
$$

where $\bar{r}$ is the sample mean of daily returns and $\sigma_r$ is
the population standard deviation.

### Annualised Sortino Ratio

$$
\text{Sortino} = \frac{\bar{r}}{\sigma_{\text{down}}} \cdot \sqrt{Y}
$$

where $\sigma_{\text{down}}$ is the standard deviation of negative daily
returns only — upside volatility is not penalised.

### Max Drawdown

$$
\text{MDD} = \min_{0 \le i \le T} \left(\frac{P_i - \max_{0 \le j \le i} P_j}{\max_{0 \le j \le i} P_j}\right)
$$

The worst peak‑to‑trough decline over the simulation horizon, expressed
as a negative number.

### Calmar Ratio

$$
\text{Calmar} = \frac{\text{CAGR}}{|\text{MDD}|}
$$

### Annualised Volatility

$$
\text{Volatility} = \sigma_r \cdot \sqrt{Y}
$$

All seven metrics are computed in C and returned in‑memory per
combination via the `performance_t` struct.

---

## Installation

```bash
# 1. Clone the repository
git clone https://github.com/janneschoen/strat-optimizer.git
cd strat-optimizer

# 2. Build the C engine (requires GCC with OpenMP)
#    `make` builds both libengine.so (used by Python) and engine-cli
make

# 3. Create and activate a virtual environment
python -m venv .venv
source .venv/bin/activate          # Linux / macOS
# .venv\Scripts\activate           # Windows

# 4. Install Python dependencies
pip install -r requirements.txt

# 5. (optional) install test dependencies and run the suite
pip install -r requirements-dev.txt
pytest
```

**Dependencies:**
- C compiler with OpenMP support (GCC or Clang)
- Python ≥ 3.8
- Core Python packages: `numpy`, `matplotlib`, `yfinance`

---

## Quick Start

### 1. Create `configs/config.json`

```json
{
    "strategy_name": "SMA Crossover",
    "strategies_file": "configs/strategies.json",

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
python -m strat_optimizer                    # uses configs/config.json
python -m strat_optimizer my_config.json     # custom config file
```

### 3. Output

The program uses `rich` for formatted terminal output.  Here is what a
run looks like with the example config above:

```

╭──────────────────────────────────────────╮
│  Strat‑Optimizer  ·  Grid‑search         │
│  backtesting engine                      │
╰──────────────────────────────────────────╯

╭─ Configuration ─────────────────────────╮
│ Config file         configs/config.json │
│ Strategy            SMA Crossover       │
│ Asset               BTC-USD  (Crypto/   │
│                     Forex (365 d/y))     │
│ Backtest length     2,000 trading days  │
│ Train / test split  60% / 40%           │
│ Lookback            200 days            │
╰─────────────────────────────────────────╯

  Downloading prices … done  (1.2s)
    2022-05-14 → 2025-01-12  │  2,200 prices  │  2,000 usable trading days

╭─ Parameter Grid ────────────────────────╮
│ Fast SMA Length    [1 … 200]  step=3   │
│                    →  67 values         │
│ Slow SMA Length    [30 … 200]  step=3  │
│                    →  57 values         │
│ Position Sizing   0.2                   │
│ (fixed)                                 │
│                                         │
│ Combinations       1,910 valid          │
│ combinations  (3,819 total, 1,909       │
│ pruned by constraints)                  │
╰─────────────────────────────────────────╯

  Training on 1,200 days with 1,910 combinations … done  (18.3s, 2,292,000 days simulated, 125,246 d/s)
  Testing  on 800 days (walk‑forward) … done  (0.6s)

╭─ Best Parameters ───────────────────────╮
│ Fast SMA Length    1                    │
│ Slow SMA Length    120                  │
│ Position Sizing    0.2                  │
╰─────────────────────────────────────────╯

╭─ Strategy Performance ──────────────────────────────────────────────────────╮
│           Sharpe  Sortino  Ann.Prof   Max DD   Calmar    Vol.              │
│ Training  2.1431  +2.0105  +38.40%   -18.23%   +2.1062   12.45%           │
│ Testing   1.8712  +1.7533  +31.27%   -16.89%   +1.8514   10.92%           │
│                                                                             │
│ Δ train→  -0.2719  -0.2572   -7.1pp    +1.3pp   -0.2548   -1.5pp          │
│ test                                                                        │
│ Total return — Training: 45.12%  Testing: 32.80%                            │
╰─────────────────────────────────────────────────────────────────────────────╯

╭─ Buy & Hold Benchmark ───────────────────────────────────────────────────────╮
│           Sharpe   Ann. Profit                                              │
│ Training  0.8124      +12.15%                                               │
│ Testing   0.7501       +9.83%                                               │
╰─────────────────────────────────────────────────────────────────────────────╯

╭─ Timing ────────────────────────────────╮
│ Config loading              0.0s        │
│ Price download              1.2s        │
│ Grid generation             0.0s        │
│ Training phase             18.3s    88% │
│ Testing phase               0.6s     3% │
│                                          │
│ Total                      20.8s         │
╰─────────────────────────────────────────╯

  Opening matplotlib figures …
  Done.

```

After the text output, interactive Matplotlib figures open:
- Heatmap of Sharpe ratio / annual profit vs parameters (training window)
- Equity curve of the best combination (test window)

---

## Configuration Reference

### `config.json`

| Key | Type | Description |
|-----|------|-------------|
| `strategy_name` | string | Name matching an entry in `strategies.json` |
| `strategies_file` | string | Path to the strategy definitions file |
| `parameter_ranges` | `[[low, high], ...]` | One `[low, high]` pair per strategy parameter — the single source of the numeric search space (the high end is exclusive, like `np.arange`) |
| `parameter_steps` | `[int or float, ...]` | Grid step size per parameter; `0` = fixed at `range[0]` |
| `backtest_length` | int | Number of **trading days** to test (from yesterday backwards) |
| `test_size` | float | Fraction of data held out for walk‑forward testing `[0, 1]` |
| `asset.ticker` | string | Yahoo Finance ticker (e.g. `BTC-USD`, `AAPL`) |
| `asset.is_traded_all_year` | bool | `true` for crypto/forex (365 d/y), `false` for equities (252 d/y) |
| `transaction_cost` | float | Fraction of traded value deducted per rebalance (default `0.0`).  E.g. `0.001` = 10 bps |
| `show_plots` | bool | Whether to open interactive Matplotlib figures after the run (default `true`) |

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
| `upper_param` | int | no | Index of a parameter this value must be **strictly less than** (e.g. Fast SMA < Slow SMA) |
| `defines_lookback` | bool | no | Exactly one parameter per strategy must have this; its maximum value determines how many historical prices the signal function receives |

Numeric bounds are **not** declared here — the search space is fully
defined by `parameter_ranges` / `parameter_steps` in the run config.
`strategies.json` only carries structural metadata (names, relational
constraints, the lookback parameter).

**Example** (`strategies.json`):

```json
[
    {
        "name": "SMA Crossover",
        "parameters": [
            {"name": "Fast SMA Length", "upper_param": 1},
            {"name": "Slow SMA Length", "defines_lookback": true},
            {"name": "Position Sizing"}
        ]
    },
    {
        "name": "RSI",
        "parameters": [
            {"name": "Buying Threshold",  "upper_param": 1},
            {"name": "Selling Threshold"},
            {"name": "Window Size",       "defines_lookback": true}
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
│    • Store as numpy float32 array in memory              │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 3. Generate parameter grid  (parameters.py)              │
│    • Cartesian product of per‑parameter ranges           │
│    • Filter through strategy.is_valid() constraints      │
│    • Flatten into numpy float32 array                    │
└─────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│ 4. Train  (C engine, first 1−test_size of data)          │
│    • Call engine_run() via ctypes (in-process)           │
│    • Pass price array + grid via float* pointers         │
│    • Each combination → backtest() → performance_t       │
│    • Single combo: equity curve returned in buffer       │
│    • Multiple combos: OpenMP parallel, equity discarded  │
│    • Read results from numpy output arrays               │
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
│    • Return equity curve in numpy array for plotting     │
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

Adding a new strategy requires three steps: a C signal function, a
registry entry (by name), and a JSON metadata entry.

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
    // Access historical prices (lookahead-safe):
    //   prices[day], prices[day - 1], ... are real;
    //   prices[i] for i > day is NAN.  Do not peek forward.

    // Return one of:
    //   > 0            target long exposure, fraction of net worth
    //   < 0            target short exposure, fraction of net worth
    //   SIGNAL_HOLD    keep the current position unchanged
    //   SIGNAL_FLAT    close any open position and stay in cash

    return SIGNAL_HOLD;
}
```

**Contract:**

| Aspect | Rule |
|--------|------|
| **Return value** | An exposure (`float`), `SIGNAL_HOLD` (0.0), or `SIGNAL_FLAT` (NaN). `SIGNAL_HOLD` is *not* the same as flat — the engine leaves the current position open. |
| **Price data** | `prices[i]` for `i ≤ day` is real; `prices[i]` for `i > day` is `NAN`.  Do **not** peek forward. |
| **State** | Use `strat->storage[]` for day‑to‑day state.  The engine resets it to NAN between combinations. |
| **Cost model** | Transaction costs are deducted from cash on each rebalance (see `transaction_cost` in config).  No slippage. |
| **Wipeout** | If net worth drops to ≤ 0, the engine stops simulating and zeros the remainder of the equity curve. |

### 2. Register the Function

Declare the function in `common.h`:

```c
float signal_MyStrategy(unsigned day,
                        strategy_config_t * strat,
                        float * prices);
```

Append a registry entry in `backtesting.c`:

```c
static const strategy_entry_t STRATEGY_REGISTRY[] = {
    { "SMA Crossover", signal_SMA_crossover },
    { "RSI",           signal_RSI },
    { "My Strategy",   signal_MyStrategy },   // ← append here
};
```

The registry key must match both `config.strategy_name` and the `name`
in `strategies.json`.  Order does **not** matter — the engine dispatches
by name via `strategy_lookup()`, so the table and the JSON can evolve
independently.

### 3. Define Properties (JSON)

Add an entry to `strategies.json`:

```json
{
    "name": "My Strategy",
    "parameters": [
        {"name": "Param A", "upper_param": 1},
        {"name": "Param B", "defines_lookback": true},
        {"name": "Param C"}
    ]
}
```

The numeric search space (and therefore `Param A`'s lower bound) comes
from `parameter_ranges` / `parameter_steps` in the run config.

**Constraints you can express:**

| Constraint | JSON key | Example |
|------------|----------|---------|
| Must be less than another param | `"upper_param"` | Param A < Param B |
| Defines required lookback | `"defines_lookback"` | The engine provides at least `max(Param B)` historical prices |

`upper_param` is enforced during grid generation; any additional
validation logic belongs in the C signal function.  A strategy may define
at most `MAX_PARAMS` (3) parameters — enough for the 3‑D visualisations —
and the Python layer rejects anything larger before it reaches C.

### 4. Build & Run

```bash
make              # recompile with the new .c file
python -m strat_optimizer    # run with a config that references "My Strategy"
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

Benchmarked on an Intel i7-10610U (4C/8T, 1.8 GHz base, 4.9 GHz boost),
~1340 events/s on `sysbench cpu run`.  Compiled with `gcc -O2 -march=native
-fopenmp`.  All runs use BTC‑USD with a 60/40 train/test split, so training
days = 60 % of the backtest window.

| Strategy | Combos | Training days | Days simulated | Wall time | Throughput |
|----------|--------|--------------|---------------|-----------|------------|
| SMA Crossover | 2,166 | 1,200 | 2.6 M | 0.07 s | ~38 M d/s |
| RSI | 1,280 | 1,200 | 1.5 M | 0.04 s | ~42 M d/s |

Numbers are median of 5 consecutive runs after one warm‑up call.  The
Python layer (config parsing, data download, grid generation, plotting)
contributes negligible overhead — over 99 % of total runtime is spent
in the C engine during grid search.

OpenMP scaling is near‑linear on machines with ≤ 8 physical cores for
typical grid sizes.

### What affects simulation speed

- **Lookback length** — longer windows mean more SMA/RSI computation per day
- **Strategy complexity** — simple arithmetic (SMA) vs iterative indicators (RSI)
- **Grid density** — doubling the number of combinations roughly doubles runtime
- **Price series length** — linear in the number of days simulated

---

## Project Structure

```
strat-optimizer/
├── strat_optimizer/         # Python package
│   ├── __init__.py
│   ├── __main__.py          # Entry point for `python -m strat_optimizer`
│   ├── main.py              # Pipeline orchestrator
│   ├── config.py            # JSON → RunConfig dataclass
│   ├── parameters.py        # Cartesian product grid with constraint filtering
│   ├── prices.py            # Yahoo Finance downloader
│   ├── backtesting.py       # Python ↔ C bridge (ctypes + shared library)
│   ├── strategy.py          # Strategy metadata & parameter validation
│   ├── plotting.py          # 2‑D/3‑D visualisation dispatch
│   └── equity_curve.py      # Equity curve plot with linear trend
│
├── src/                     # C engine
│   ├── common.h             # Shared C types: performance_t, strategy_config_t, run_config_t
│   ├── engine.h             # Public API: engine_args_t struct + engine_run() signature
│   ├── engine.c             # Shared-library entry point: receives data via pointers, runs grid
│   ├── config.c             # CLI key:value parser (populates run_config_t)
│   ├── backtesting.c        # Core simulation loop + name-based strategy registry
│   ├── core.c               # Standalone CLI driver (for debugging; not used by Python)
│   └── strategies/
│       ├── 01-SMA-Crossover.c  # Strategy: Simple Moving Average crossover
│       └── 02-RSI.c            # Strategy: Relative Strength Index
│
├── libengine.so             # Compiled shared library (ctypes entry point for Python)
├── engine-cli               # Standalone CLI binary (for manual debugging)
├── configs/                 # JSON configuration
│   ├── strategies.json      # Strategy definitions (names, relational constraints)
│   ├── example.json         # Example run configuration
│   └── config.json          # Active run configuration (user‑provided, gitignored)
├── tests/                   # Pytest suite (grid, config, engine, downloader)
├── requirements.txt         # Python dependencies
├── requirements-dev.txt     # Test dependencies (pytest)
└── Makefile                 # C build (GCC + OpenMP)
```

---

## Ideas

- **Multi‑asset.** Run the same strategy (or different ones) across several
  tickers and combine the equity curves into a portfolio.  Most of the
  plumbing is already there — the C engine just needs a loop over assets.
