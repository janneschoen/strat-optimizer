/*
 *  02-RSI.c — Relative Strength Index strategy
 *
 *  Signal logic:
 *    Compute the RSI over a configurable lookback window.
 *    If RSI drops below the buy threshold   → go long  (oversold).
 *    If RSI rises above the sell threshold  → go short (overbought).
 *    Otherwise stay flat.
 *
 *    Unlike SMA Crossover, this strategy does not track state
 *    between days — each day's signal is independent.
 *
 *  RSI formula (Wilder's smoothed variant simplified to SMA):
 *    RSI = 100 − 100 / (1 + average_gain / average_loss)
 *
 *  Edge cases:
 *    No movement in window      → RSI = 50 (neutral)
 *    All gains, no losses       → RSI = 100
 *    All losses, no gains       → RSI = 0
 *
 *  Parameters:
 *    params[0]  Buy threshold  (RSI below this → long)   ∈ [0, 100]
 *    params[1]  Sell threshold (RSI above this → short)  ∈ [0, 100]
 *    params[2]  Window size    (defines the lookback)     ≥ 1
 *
 *  Constraint: buy_threshold < sell_threshold (enforced by JSON).
 */

#include "common.h"
#include <stdio.h>

float signal_RSI(unsigned day,
                 strategy_config_t * strat,
                 float * prices)
{
    float gain_sum = 0.0f;
    float loss_sum = 0.0f;

    unsigned n_gains  = 0;
    unsigned n_losses = 0;

    unsigned buy_thresh  = (unsigned)strat->params[0];
    unsigned sell_thresh = (unsigned)strat->params[1];
    unsigned window      = (unsigned)strat->params[2];

    /* accumulate gains and losses over the lookback window */
    for(unsigned i = day - window; i < day; i++){
        float change = prices[i + 1] - prices[i];
        if(change > 0){
            gain_sum += change;
            n_gains++;
        } else{
            loss_sum -= change;    // loss_sum stores positive magnitudes
            n_losses++;
        }
    }

    /* compute RSI, handling degenerate cases */
    float rsi;
    if(n_gains > 0 && n_losses > 0){
        float avg_gain = gain_sum / (float)n_gains;
        float avg_loss = loss_sum / (float)n_losses;
        rsi = 100.0f / (1.0f + (avg_gain / avg_loss));
    } else if(n_gains + n_losses == 0){
        rsi = 50.0f;               // no price movement — neutral
    } else if(n_gains == 0){
        rsi = 0.0f;                // all losses
    } else{
        rsi = 100.0f;              // all gains
    }

    if(rsi < (float)buy_thresh)   return  1.0f;
    if(rsi > (float)sell_thresh)  return -1.0f;
    return 0.0f;                   // inside the neutral zone
}
