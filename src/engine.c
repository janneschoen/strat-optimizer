/*
 *  engine.c — Shared-library entry point for the backtesting engine
 *
 *  Contains the public API: engine_run() plus a few ABI-introspection
 *  helpers used by the Python bridge.  It receives all data via
 *  pointers (no files, no CLI parsing), runs the grid of backtests
 *  in-place, and writes results back through the same pointer-based
 *  interface.
 *
 *  This is the hot path called by Python via ctypes.  The old
 *  file-based CLI in core.c also calls this function after loading
 *  data from disk.
 */

#include "engine.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRESS_INTV 100   /* print a status tick every N combos */

size_t   engine_args_sizeof(void) { return sizeof(engine_args_t); }
unsigned engine_num_metrics(void) { return NUM_PERFORMANCE_METRICS; }
unsigned engine_max_params(void)  { return MAX_PARAMS; }

void engine_run(engine_args_t *args)
{
    unsigned n_combos = args->n_combos;
    unsigned n_params = args->n_params;
    unsigned days     = args->end - args->start;

    /* deterministic output even if we bail out early below */
    if (args->performances != NULL) {
        memset(args->performances, 0,
               n_combos * NUM_PERFORMANCE_METRICS * sizeof(float));
    }

    /* defensive guard: params[] in strategy_config_t is MAX_PARAMS wide */
    if (n_params > MAX_PARAMS) {
        fprintf(stderr,
                "engine_run: strategy has %u parameters, but the engine "
                "supports at most %u (MAX_PARAMS).\n",
                n_params, MAX_PARAMS);
        return;
    }

    /* resolve the strategy by name — no fragile positional index */
    signal_fn_t signal_fn = strategy_lookup(args->strategy_name);
    if (signal_fn == NULL) {
        fprintf(stderr, "engine_run: unknown strategy '%s'.\n",
                args->strategy_name ? args->strategy_name : "(null)");
        return;
    }

    /* ---- build strategy_config_t for each combination ----------- */
    strategy_config_t *combinations =
        malloc(n_combos * sizeof(strategy_config_t));
    if (combinations == NULL) {
        fprintf(stderr, "engine_run: out of memory.\n");
        return;
    }

    for (unsigned i = 0; i < n_combos; i++) {
        for (unsigned j = 0; j < n_params; j++) {
            combinations[i].params[j] =
                args->param_grid[i * n_params + j];
        }
    }

    /* ---- populate a minimal run_config_t for backtest() --------- */
    run_config_t run;
    memset(&run, 0, sizeof(run));
    run.start                  = args->start;
    run.end                    = args->end;
    run.number_of_prices       = args->n_prices;
    run.number_of_parameters   = n_params;
    run.number_of_combinations = n_combos;
    run.signal_fn              = signal_fn;
    run.trading_days           = args->trading_days;
    run.transaction_cost       = args->transaction_cost;
    /* file-path fields are unused by backtest() — left zeroed */

    /* ---- backtest every combination ----------------------------- */
    if (n_combos == 1) {
        /*
         * Single combination: run sequentially.
         * Equity curve is saved if the caller provided a buffer.
         */
        float *equity_curve = malloc(days * sizeof(float));
        if (equity_curve == NULL) {
            fprintf(stderr, "engine_run: out of memory.\n");
            free(combinations);
            return;
        }
        backtest(run, &combinations[0], args->prices, equity_curve);

        if (args->equity_curve) {
            memcpy(args->equity_curve, equity_curve,
                   days * sizeof(float));
        }
        free(equity_curve);

    } else {
        /*
         * Multiple combinations: parallel via OpenMP.
         * schedule(dynamic) helps when some parameter combos
         * run slower than others (e.g. longer lookbacks).
         *
         * Each thread gets its own heap equity buffer → zero data
         * races.  Progress is printed inside a critical section.
         */
        unsigned completed = 0;
        #pragma omp parallel for schedule(dynamic)
        for (unsigned i = 0; i < n_combos; i++) {
            float *equity_curve = malloc(days * sizeof(float));
            if (equity_curve != NULL) {
                backtest(run, &combinations[i], args->prices,
                         equity_curve);
                free(equity_curve);
            } else {
                memset(&combinations[i].performance, 0,
                       sizeof(performance_t));
            }

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
        unsigned base = i * NUM_PERFORMANCE_METRICS;
        performance_t *p = &combinations[i].performance;
        args->performances[base + 0] = p->annual_profit;
        args->performances[base + 1] = p->sharpe_ratio;
        args->performances[base + 2] = p->total_return;
        args->performances[base + 3] = p->max_drawdown;
        args->performances[base + 4] = p->sortino_ratio;
        args->performances[base + 5] = p->calmar_ratio;
        args->performances[base + 6] = p->volatility;
    }

    free(combinations);
}
