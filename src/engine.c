/*
 *  engine.c — Shared-library entry point for the backtesting engine
 *
 *  Contains exactly one public function: engine_run().  It receives
 *  all data via pointers (no files, no CLI parsing), runs the grid
 *  of backtests in-place, and writes results back through the same
 *  pointer-based interface.
 *
 *  This is the hot path called by Python via ctypes.  The old
 *  file-based CLI in core.c also calls this function after loading
 *  data from disk.
 */

#include "engine.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

#define PROGRESS_INTV 100   /* print a status tick every N combos */

void engine_run(engine_args_t *args)
{
    unsigned n_combos = args->n_combos;
    unsigned n_params = args->n_params;
    unsigned n_prices = args->n_prices;
    unsigned days     = args->end - args->start;

    /* ---- copy prices into a local buffer ------------------------ */
    float prices[n_prices];
    memcpy(prices, args->prices, n_prices * sizeof(float));

    /* ---- build strategy_config_t for each combination ----------- */
    strategy_config_t combinations[n_combos];

    for (unsigned i = 0; i < n_combos; i++) {
        for (unsigned j = 0; j < n_params; j++) {
            combinations[i].params[j] =
                args->param_grid[i * n_params + j];
        }
    }

    /* ---- populate a minimal run_config_t for backtest() --------- */
    run_config_t run;
    run.start                = args->start;
    run.end                  = args->end;
    run.number_of_prices     = n_prices;
    run.number_of_parameters = n_params;
    run.number_of_combinations = n_combos;
    run.strategy_index       = args->strategy_index;
    run.trading_days         = args->trading_days;
    /* file-path fields are unused by backtest() — leave zeroed */

    /* ---- backtest every combination ----------------------------- */
    if (n_combos == 1) {
        /*
         * Single combination: run sequentially.
         * Equity curve is saved if the caller provided a buffer.
         */
        float equity_curve[days];
        backtest(run, &combinations[0], prices, equity_curve);

        if (args->equity_curve) {
            memcpy(args->equity_curve, equity_curve,
                   days * sizeof(float));
        }


    } else {
        /*
         * Multiple combinations: parallel via OpenMP.
         * schedule(dynamic) helps when some parameter combos
         * run slower than others (e.g. longer lookbacks).
         *
         * Each thread gets its own equity_curve stack buffer →
         * zero data races.  Progress is printed inside a
         * critical section.
         */
        unsigned completed = 0;
        #pragma omp parallel for schedule(dynamic)
        for (unsigned i = 0; i < n_combos; i++) {
            float equity_curve[days];
            backtest(run, &combinations[i], prices, equity_curve);

            #pragma omp critical
            {
                completed++;
                if (completed % PROGRESS_INTV == 0
                    || completed == n_combos) {
                    printf("\r%u / %u\033[K", completed, n_combos);
                    fflush(stdout);
                }
            }
        }
        printf("\r\033[K");
        fflush(stdout);
    }

    /* ---- write performance metrics to the output array ---------- */
    for (unsigned i = 0; i < n_combos; i++) {
        args->performances[i * 2]     =
            combinations[i].performance.annual_profit;
        args->performances[i * 2 + 1] =
            combinations[i].performance.sharpe_ratio;
    }
}
