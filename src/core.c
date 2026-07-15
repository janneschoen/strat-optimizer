/*
 *  core.c — Standalone CLI driver for the backtesting engine
 *
 *  Loads prices and parameter combinations from temp files, then
 *  delegates to engine_run() for the actual simulation.  Writes
 *  results back to disk.
 *
 *  This binary exists for debugging — you can run it manually:
 *      ./compute start:200 end:1200 number_of_prices:3000 ...
 *
 *  In normal use, Python calls engine_run() directly through the
 *  shared library (libengine.so), bypassing this CLI wrapper.
 */

#include "common.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf("Program has to be launched via main.py\n");
        return 1;
    }

    run_config_t run = load_config(argc, argv);

    unsigned days = run.end - run.start;

    /* ---- load price time-series ---------------------------------- */
    float *prices = malloc(run.number_of_prices * sizeof(float));
    if (!prices) { printf("Error: out of memory\n"); return 1; }

    FILE *file = fopen(run.prices_path, "r");
    if (!file) {
        printf("Error: Could not open prices file '%s'.\n",
               run.prices_path);
        free(prices);
        return 1;
    }
    for (unsigned i = 0; i < run.number_of_prices; i++) {
        if (fscanf(file, "%f", &prices[i]) != 1) {
            printf("Error: Prices could not be properly read. "
                   "Possible reason: not enough prices available "
                   "for chosen timeframe.\n");
            fclose(file);
            free(prices);
            return 1;
        }
    }
    fclose(file);

    /* ---- load parameter combinations (grid) ---------------------- */
    unsigned n_grid = run.number_of_combinations
                      * run.number_of_parameters;
    float *param_grid = malloc(n_grid * sizeof(float));
    if (!param_grid) {
        printf("Error: out of memory\n");
        free(prices);
        return 1;
    }

    file = fopen(run.parameter_path, "r");
    if (!file) {
        printf("Error: Could not open parameters file '%s'.\n",
               run.parameter_path);
        free(prices);
        free(param_grid);
        return 1;
    }
    for (unsigned i = 0; i < n_grid; i++) {
        if (fscanf(file, "%f", &param_grid[i]) != 1) {
            printf("Error: Parameter combinations could not be "
                   "properly read.\n");
            fclose(file);
            free(prices);
            free(param_grid);
            return 1;
        }
    }
    fclose(file);

    /* ---- allocate outputs --------------------------------------- */
    float *performances =
        malloc(run.number_of_combinations * 2 * sizeof(float));
    float *equity_curve = NULL;

    if (run.number_of_combinations == 1) {
        equity_curve = malloc(days * sizeof(float));
    }

    if (!performances || (run.number_of_combinations == 1
                          && !equity_curve)) {
        printf("Error: out of memory\n");
        free(prices);
        free(param_grid);
        free(performances);
        free(equity_curve);
        return 1;
    }

    /* ---- delegate to the shared engine -------------------------- */
    engine_args_t args = {
        .prices         = prices,
        .param_grid     = param_grid,
        .performances   = performances,
        .equity_curve   = equity_curve,
        .n_prices       = run.number_of_prices,
        .n_combos       = run.number_of_combinations,
        .n_params       = run.number_of_parameters,
        .strategy_index = run.strategy_index,
        .start          = run.start,
        .end            = run.end,
        .trading_days   = run.trading_days,
    };

    engine_run(&args);

    /* ---- write the equity curve (single-combo only) -------------- */
    if (equity_curve) {
        file = fopen(run.equity_path, "w");
        if (file) {
            for (unsigned i = 0; i < days; i++) {
                fprintf(file, "%f\n", equity_curve[i]);
            }
            fclose(file);
        }
        free(equity_curve);
    }

    /* ---- write performance metrics to disk ----------------------- */
    file = fopen(run.performances_path, "w");
    if (!file) {
        printf("Error: Could not write performances to '%s'.\n",
               run.performances_path);
        free(prices);
        free(param_grid);
        free(performances);
        return 1;
    }
    for (unsigned i = 0; i < run.number_of_combinations; i++) {
        fprintf(file, "%f, %f\n",
                performances[i * 2], performances[i * 2 + 1]);
    }
    fclose(file);

    free(prices);
    free(param_grid);
    free(performances);

    return 0;
}
