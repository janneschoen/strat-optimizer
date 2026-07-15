/*
 *  engine.h — Public API for the backtesting engine shared library
 *
 *  This is the interface Python calls into (via ctypes) to run
 *  backtests in-process.  All data flows through pointers — no
 *  files, no subprocesses, no serialization.
 */

#ifndef ENGINE_H
#define ENGINE_H

typedef struct {
    float   *prices;           /* in:  price series [n_prices]              */
    float   *param_grid;       /* in:  flat grid [n_combos * n_params]      */
    float   *performances;     /* out: [annual_profit, sharpe] × n_combos   */
    float   *equity_curve;     /* out: equity curve [end-start], NULL ok    */
    unsigned n_prices;
    unsigned n_combos;
    unsigned n_params;
    unsigned strategy_index;   /* index into get_signal[] dispatch table    */
    unsigned start;            /* first tradable day                        */
    unsigned end;              /* one past the last tradable day            */
    unsigned trading_days;     /* 252 (equities) or 365 (crypto)            */
} engine_args_t;

/*
 *  Run a grid of backtests.
 *
 *  When n_combos == 1, the equity_curve output is populated (if
 *  non-NULL) and the run is single-threaded.
 *
 *  When n_combos > 1, OpenMP parallelises across combinations and
 *  equity_curve is ignored (set it to NULL).  Only aggregate
 *  performance metrics are written.
 */
void engine_run(engine_args_t *args);

#endif
