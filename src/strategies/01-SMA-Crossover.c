/*
 *  01-SMA-Crossover.c — Simple Moving Average Crossover strategy
 *
 *  Signal logic:
 *    Compute two SMAs every day: a fast one and a slow one.
 *    When the fast SMA crosses above the slow  → go long.
 *    When the fast SMA crosses below the slow  → go short.
 *
 *    Only acts on crossover events — holds the position until the
 *    next crossover.  Position size is controlled by params[2],
 *    a fixed fraction of net worth (e.g. 0.2 = 20 %).
 *
 *  Parameters:
 *    params[0]  Fast SMA window length  (must be < params[1])
 *    params[1]  Slow SMA window length  (defines the lookback)
 *    params[2]  Position size fraction  ∈ [0, 1]
 *
 *  State:
 *    storage[0] stores the previous crossover direction so we can
 *    detect reversals.  Reset to NAN by the engine between runs.
 */

#include "common.h"
#include <math.h>
#include <stdio.h>

/* Arithmetic mean of prices[from .. to-1] */
static float sma(float * prices, unsigned from, unsigned to){
    float sum = 0.0f;
    for(unsigned i = from; i < to; i++){
        sum += prices[i];
    }
    return sum / (float)(to - from);
}

float signal_SMA_crossover(unsigned day,
                           strategy_config_t * strat,
                           float * prices)
{
    unsigned fast_len = (unsigned)strat->params[0];
    unsigned slow_len = (unsigned)strat->params[1];

    float fast_sma = sma(prices, day - fast_len, day);
    float slow_sma = sma(prices, day - slow_len, day);

    int direction   = (fast_sma > slow_sma) ? 1 : -1;
    int prev_dir    = isnan(strat->storage[0]) ? 0 : (int)strat->storage[0];
    float alloc     = strat->params[2];

    if(direction != prev_dir){
        strat->storage[0] = (float)direction;
        return alloc * (float)direction;   // enter position on crossover
    }

    return 0.0f;   // no crossover today — hold
}
