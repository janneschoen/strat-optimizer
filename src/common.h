#ifndef COMMON_H
#define COMMON_H

/*
 *  common.h — Shared type definitions and function signatures
 *
 *  Interface contract between:
 *    - the strategy signal functions    (src/strategies/)
 *    - the backtesting engine           (backtesting.c)
 *    - the shared-library entry point   (engine.c)
 *    - the standalone CLI               (core.c + config.c)
 *
 *  The Python orchestrator (main.py → backtesting.py) loads the compiled
 *  libengine.so via ctypes and calls engine_run() in-process.  All data
 *  flows through numpy arrays in shared memory — no subprocess, no temp
 *  files.  The standalone 'engine-cli' binary (core.c) is kept for manual
 *  debugging only.
 */

#include <math.h>   /* NAN — used by SIGNAL_FLAT and the strategies */

#define MAX_PARAMS      3    // hard cap on strategy parameters (engine + plots)
#define STRAT_STORAGE   1    // per-combination scratch slots for stateful strategies
#define MAX_VALUE_LENGTH 64  // max length of a single CLI key or value string

#define NUM_PERFORMANCE_METRICS 7  // floats per combo in the output array

/* ------------------------------------------------------------------ */
/*  Signal contract (return value of a strategy signal function)       */
/*                                                                     */
/*    > 0            target long  exposure, fraction of net worth       */
/*    < 0            target short exposure, fraction of net worth       */
/*    SIGNAL_HOLD    keep the current position unchanged                */
/*    SIGNAL_FLAT    close any open position and stay in cash           */
/*                                                                     */
/*  The engine only trades when the target changes, so a strategy that  */
/*  wants to "hold between signals" returns SIGNAL_HOLD, while one that */
/*  genuinely wants to be flat returns SIGNAL_FLAT.                     */
/* ------------------------------------------------------------------ */
#define SIGNAL_HOLD  (0.0f)
#define SIGNAL_FLAT  (NAN)

/* ------------------------------------------------------------------ */
/*  performance_t                                                     */
/*                                                                     */
/*  Risk/reward outcome of a single backtest run.  All ratio /        */
/*  return metrics are annualized so results across different time     */
/*  horizons are directly comparable.                                  */
/* ------------------------------------------------------------------ */
typedef struct {
    float sharpe_ratio;    // annualized Sharpe: (μ − RF) / σ × √td
    float annual_profit;   // CAGR: (final / initial)^(td/days) − 1
    float total_return;    // simple period return: (final − init) / init
    float max_drawdown;    // worst peak-to-trough decline (negative)
    float sortino_ratio;   // annualized Sortino: (μ − RF) / σ_down × √td
    float calmar_ratio;    // annual_profit / |max_drawdown|
    float volatility;      // annualized std dev of daily returns
} performance_t;


/* ------------------------------------------------------------------ */
/*  strategy_config_t                                                 */
/*                                                                     */
/*  Holds ONE parameter combination + its results.  The 'storage'     */
/*  array is scratch space for stateful strategies (e.g. SMA          */
/*  crossover remembers the current regime).  It is reset to NAN by    */
/*  the engine before each backtest.                                   */
/* ------------------------------------------------------------------ */
typedef struct {
    float          params[MAX_PARAMS];
    performance_t  performance;
    float          storage[STRAT_STORAGE];
} strategy_config_t;


/* ---- signal function + name-based registry ----------------------- */

/*  day     : index of the current (just-revealed) price
 *  strategy: this combination's params + scratch storage
 *  prices  : price series; entries > day are NAN (no lookahead)       */
typedef float (*signal_fn_t)(unsigned day,
                             strategy_config_t * strategy,
                             float * prices);

/*  Registry entry: the strategy's public name and its function.
 *  The table lives in backtesting.c; dispatch is by name so the C
 *  registry and strategies.json can be reordered independently.       */
typedef struct {
    const char * name;
    signal_fn_t  fn;
} strategy_entry_t;

/*  Resolve a registered strategy by name; returns NULL if unknown.   */
signal_fn_t strategy_lookup(const char * name);


/* ------------------------------------------------------------------ */
/*  run_config_t                                                      */
/*                                                                     */
/*  All the metadata a backtest needs — horizon, data locations,      */
/*  which strategy to use, and how many combinations to evaluate.     */
/*  Populated by load_config() (CLI) or engine_run() (Python bridge). */
/* ------------------------------------------------------------------ */
typedef struct {
    unsigned start;                    // first tradable index (≥ lookback)
    unsigned end;                      // one past the last tradable index

    unsigned number_of_prices;
    unsigned number_of_parameters;
    unsigned number_of_combinations;

    char         strategy_name[MAX_VALUE_LENGTH];  // registry key
    signal_fn_t  signal_fn;                        // resolved dispatch target
    unsigned     trading_days;                     // 252 (equities) or 365 (crypto)
    float        transaction_cost;                 // fraction of traded value (0 = disabled)

    char prices_path[MAX_VALUE_LENGTH];
    char parameter_path[MAX_VALUE_LENGTH];
    char equity_path[MAX_VALUE_LENGTH];
    char performances_path[MAX_VALUE_LENGTH];
} run_config_t;


/* ---- function declarations --------------------------------------- */

run_config_t load_config(int argc, char * argv[]);

void backtest(run_config_t run,
              strategy_config_t * strategy,
              float * prices,
              float * equity_curve);

/* Strategy signal functions — each returns the value described by the
 * signal contract above (exposure, SIGNAL_HOLD, or SIGNAL_FLAT).       */
float signal_SMA_crossover(unsigned day,
                           strategy_config_t * strategy,
                           float * prices);

float signal_RSI(unsigned day,
                 strategy_config_t * strategy,
                 float * prices);

#endif
