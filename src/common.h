#ifndef COMMON_H
#define COMMON_H

/*
 *  common.h — Shared type definitions and function signatures
 *
 *  This header is the interface contract between:
 *    - the strategy signal functions   (01-*.c, 02-*.c)
 *    - the backtesting engine          (backtesting.c)
 *    - the CLI argument parser         (config.c)
 *    - the main driver                 (core.c)
 *    - the shared-library entry point  (engine.c)
 *
 *  The Python orchestrator (main.py → backtesting.py) loads the compiled
 *  libengine.so via ctypes and calls engine_run() in-process.  All data
 *  flows through numpy arrays in shared memory — no subprocess, no temp
 *  files.  The standalone 'compute' binary (core.c) is kept for manual
 *  debugging only.
 */

#define MAX_PARAMS      3    // hard cap on strategy parameters (visualisation limit)
#define STRAT_STORAGE   1    // per-combination scratch slots for stateful strategies
#define MAX_VALUE_LENGTH 64  // max length of a single CLI key or value string


/* ------------------------------------------------------------------ */
/*  performance_t                                                     */
/*                                                                     */
/*  Describes the risk/reward outcome of a single backtest run.        */
/*  Both metrics are annualized so results across different time       */
/*  horizons are directly comparable.                                  */
/* ------------------------------------------------------------------ */
typedef struct {
    float sharpe_ratio;    // annualized Sharpe: (μ_daily − RF) / σ_daily × √252
    float annual_profit;   // CAGR: (final / initial)^(252/days) − 1
} performance_t;


/* ------------------------------------------------------------------ */
/*  strategy_config_t                                                 */
/*                                                                     */
/*  Holds ONE parameter combination + its results.  The 'storage'     */
/*  array is scratch space for stateful strategies (e.g. SMA          */
/*  crossover remembers yesterday's crossover direction).  It is      */
/*  reset to NAN by the engine after each backtest.                   */
/* ------------------------------------------------------------------ */
typedef struct {
    float          params[MAX_PARAMS];
    performance_t  performance;
    float          storage[STRAT_STORAGE];
} strategy_config_t;


/* ------------------------------------------------------------------ */
/*  run_config_t                                                      */
/*                                                                     */
/*  All the metadata a backtest needs — horizon, data locations,      */
/*  which strategy to use, and how many combinations to evaluate.     */
/*  Populated by load_config() from CLI key:value pairs.              */
/* ------------------------------------------------------------------ */
typedef struct {
    unsigned start;                    // first tradable index (≥ lookback)
    unsigned end;                      // one past the last tradable index
    int      test_index;               // unused – reserved for future

    unsigned number_of_prices;
    unsigned number_of_parameters;
    unsigned number_of_combinations;

    unsigned strategy_index;           // index into get_signal[] table
    unsigned trading_days;             // 252 (equities) or 365 (crypto)

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

/* Strategy signal functions — each returns exposure ∈ [−1, 1]       */
/*   +1 = fully long   0 = flat   −1 = fully short                    */
float signal_SMA_crossover(unsigned day,
                           strategy_config_t * strategy,
                           float * prices);

float signal_RSI(unsigned day,
                 strategy_config_t * strategy,
                 float * prices);

#endif
