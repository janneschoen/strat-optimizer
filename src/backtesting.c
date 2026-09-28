/*
 *  backtesting.c — Core backtesting engine
 *
 *  Simulates a single parameter combination over a price series.
 *  On each day the strategy signal function returns either a target
 *  exposure, SIGNAL_HOLD (keep the current position), or SIGNAL_FLAT
 *  (close everything).  The engine rebalances the portfolio to the
 *  target whenever the strategy asks it to act.
 *
 *  See common.h for the full signal contract and the performance_t
 *  definitions of every metric written here.
 *
 *  Position model:
 *    - target > 0  → hold that fraction of net worth long
 *    - target < 0  → hold that fraction of net worth short
 *    - SIGNAL_HOLD → leave the current position untouched
 *    - SIGNAL_FLAT → close any open position and stay in cash
 *
 *  Wipeout: if net worth drops to ≤ 0, the simulation stops and
 *  the remaining equity curve is zeroed.
 */

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BUDGET          10000.0f   // initial portfolio value (arbitrary)
#define RISK_FREE_RATE  0.0f       // risk-free rate (can be set > 0)

/*
 *  Strategy registry: maps a strategy's public name to its signal
 *  function.  Dispatch is by name (strategy_lookup), so the order of
 *  this table is irrelevant and strategies.json can be reordered
 *  without silently running the wrong strategy.
 */
static const strategy_entry_t STRATEGY_REGISTRY[] = {
    { "SMA Crossover", signal_SMA_crossover },
    { "RSI",           signal_RSI },
};

#define STRATEGY_COUNT \
    (sizeof(STRATEGY_REGISTRY) / sizeof(STRATEGY_REGISTRY[0]))

signal_fn_t strategy_lookup(const char * name)
{
    if (name == NULL) {
        return NULL;
    }
    for (unsigned i = 0; i < STRATEGY_COUNT; i++) {
        if (strcmp(STRATEGY_REGISTRY[i].name, name) == 0) {
            return STRATEGY_REGISTRY[i].fn;
        }
    }
    return NULL;
}


void backtest(run_config_t run,
              strategy_config_t * strategy_config,
              float * prices,
              float * equity_curve)
{
    float cash         = BUDGET;
    float assets_owned = 0.0f;   // units of the asset held long
    float asset_loans  = 0.0f;   // units borrowed (short position)
    float networth     = 0.0f;

    /* reset strategy-local storage before this run */
    for (unsigned i = 0; i < STRAT_STORAGE; i++) {
        strategy_config->storage[i] = NAN;
    }

    unsigned start  = run.start;
    unsigned end    = run.end;
    unsigned n_days = end - start;

    /* resolve the dispatch target (engine_run normally pre-fills it) */
    signal_fn_t signal_fn = run.signal_fn;
    if (signal_fn == NULL) {
        signal_fn = strategy_lookup(run.strategy_name);
    }
    if (signal_fn == NULL || n_days == 0) {
        memset(&strategy_config->performance, 0,
               sizeof(performance_t));
        return;
    }

    /*
     * known_prices: a lookahead-safe copy of the price series.
     * Prices before 'start' are real historical data; prices from
     * 'start' onward are initially set to NAN so the strategy
     * cannot peek into the future.  Each day we reveal one more
     * price as it becomes "known".
     *
     * Heap-allocated (not a VLA) so a long backtest cannot blow the
     * thread stack, especially inside the OpenMP loop.
     */
    float * known_prices = malloc(end * sizeof(float));
    if (known_prices == NULL) {
        memset(&strategy_config->performance, 0,
               sizeof(performance_t));
        return;
    }
    for (unsigned i = 0; i < end; i++) {
        known_prices[i] = (i < start) ? prices[i] : NAN;
    }

    /* ---- main simulation loop, one iteration per trading day ---- */
    for (unsigned i = start; i < end; i++) {

        known_prices[i] = prices[i];   // reveal today's price

        /* mark-to-market net worth */
        networth = (assets_owned - asset_loans) * known_prices[i] + cash;
        equity_curve[i - start] = networth;

        if (networth <= 0.0f) {
            /* Portfolio wiped out — zero the remaining curve and stop. */
            networth = 0.0f;
            for (unsigned j = i; j < end; j++) {
                equity_curve[j - start] = 0.0f;
            }
            break;
        }

        /* query the strategy for today's desired action */
        float signal = signal_fn(i, strategy_config, known_prices);

        /*
         * Decide whether to trade and what the target position is.
         *   NaN        → SIGNAL_FLAT: close everything (target 0)
         *   0          → SIGNAL_HOLD: leave the position untouched
         *   otherwise  → target exposure = signal × net worth
         */
        int   act                = 1;
        float desired_investment = 0.0f;

        if (isnan(signal)) {
            act = 1;                              /* flatten */
        } else if (signal == SIGNAL_HOLD) {
            act = 0;                              /* hold */
        } else {
            desired_investment = signal * networth / known_prices[i];
        }

        if (act) {
            float traded_volume = 0.0f;

            if (desired_investment >= 0.0f) {
                /* ---- cover shorts, then adjust the long ---- */
                float cover_vol = asset_loans * known_prices[i];
                cash           -= cover_vol;
                traded_volume  += cover_vol;
                asset_loans     = 0.0f;

                float buy_vol = (desired_investment - assets_owned)
                                * known_prices[i];
                cash           -= buy_vol;
                traded_volume  += fabsf(buy_vol);
                assets_owned    = desired_investment;

            } else {
                /* ---- sell longs, then adjust the short ---- */
                float desired_short = fabsf(desired_investment);

                float sell_vol = assets_owned * known_prices[i];
                cash           += sell_vol;
                traded_volume  += sell_vol;
                assets_owned    = 0.0f;

                float short_vol = (desired_short - asset_loans)
                                  * known_prices[i];
                cash           += short_vol;
                traded_volume  += fabsf(short_vol);
                asset_loans     = desired_short;
            }

            /* deduct transaction cost on the traded notional */
            cash -= traded_volume * run.transaction_cost;
        }
    }  /* end of daily loop */

    free(known_prices);

    /*
     * Recompute final net worth: the value captured at the top of
     * the last iteration was pre-trade.  We now mark to market using
     * the last day's price and the post-trade cash/positions.  If the
     * portfolio wiped out (networth == 0) we skip this step.
     */
    if (networth > 0.0f) {
        networth = (assets_owned - asset_loans) * prices[end - 1] + cash;
    }

    float total_return = (networth - BUDGET) / BUDGET;

    /* ---- annualized profit (CAGR) ---- */
    if (networth > 0.0f) {
        strategy_config->performance.annual_profit =
            powf(1.0f + total_return,
                 ((float)run.trading_days / (float)n_days)) - 1.0f;
    } else {
        strategy_config->performance.annual_profit = -1.0f;
    }
    strategy_config->performance.total_return = total_return;

    /* default all ratio metrics to a neutral value */
    strategy_config->performance.sharpe_ratio  = 0.0f;
    strategy_config->performance.sortino_ratio = 0.0f;
    strategy_config->performance.volatility    = 0.0f;
    strategy_config->performance.calmar_ratio  = 0.0f;
    strategy_config->performance.max_drawdown  = 0.0f;

    unsigned number_of_returns = (n_days > 0) ? n_days - 1 : 0;
    if (number_of_returns == 0) {
        return;   /* single data point — nothing to compute */
    }

    float sum_daily_returns = 0.0f;
    float sum_sq_returns    = 0.0f;   // for volatility
    float sum_sq_downside   = 0.0f;   // for Sortino
    float peak              = equity_curve[0];
    float max_dd            = 0.0f;

    for (unsigned i = 0; i < number_of_returns; i++) {
        if (equity_curve[i] == 0.0f) {
            /* portfolio wiped out — sentinel values */
            strategy_config->performance.sharpe_ratio  = -1.0f;
            strategy_config->performance.sortino_ratio = -1.0f;
            strategy_config->performance.volatility    = -1.0f;
            strategy_config->performance.max_drawdown  = -1.0f;
            strategy_config->performance.calmar_ratio  = -1.0f;
            return;
        }

        float dr = (equity_curve[i + 1] - equity_curve[i])
                   / equity_curve[i];

        sum_daily_returns += dr;
        sum_sq_returns    += dr * dr;

        if (dr < 0.0f) {
            sum_sq_downside += dr * dr;
        }

        /* running max drawdown */
        if (equity_curve[i] > peak)
            peak = equity_curve[i];
        float dd = (equity_curve[i] - peak) / peak;
        if (dd < max_dd)
            max_dd = dd;
    }

    /* check the last equity point for drawdown */
    if (equity_curve[number_of_returns] > peak)
        peak = equity_curve[number_of_returns];
    {
        float dd = (equity_curve[number_of_returns] - peak) / peak;
        if (dd < max_dd)
            max_dd = dd;
    }
    strategy_config->performance.max_drawdown = max_dd;

    /* guard: flat equity curve */
    if (sum_daily_returns == 0.0f) {
        return;   /* sharpe/sortino/vol/calmar already 0 */
    }

    float mean_daily = sum_daily_returns / (float)number_of_returns;

    /* variance = E[r²] − E[r]²  (single-pass) */
    float variance = sum_sq_returns / (float)number_of_returns
                     - mean_daily * mean_daily;
    if (variance < 0.0f) variance = 0.0f;   // fp rounding guard
    float std_daily = sqrtf(variance);

    float ann_factor = sqrtf((float)run.trading_days);

    /* annualized volatility */
    strategy_config->performance.volatility = std_daily * ann_factor;

    /* Sharpe ratio */
    if (std_daily > 0.0f) {
        strategy_config->performance.sharpe_ratio =
            (mean_daily - RISK_FREE_RATE) / std_daily * ann_factor;
    }

    /* Sortino ratio (downside deviation) */
    float downside_dev = sqrtf(sum_sq_downside / (float)number_of_returns);
    if (downside_dev > 0.0f) {
        strategy_config->performance.sortino_ratio =
            (mean_daily - RISK_FREE_RATE) / downside_dev * ann_factor;
    }

    /* Calmar ratio */
    if (max_dd < 0.0f) {
        strategy_config->performance.calmar_ratio =
            strategy_config->performance.annual_profit / fabsf(max_dd);
    }
}
