/*
 *  backtesting.c — Core backtesting engine
 *
 *  Simulates a single parameter combination over a price series.
 *  On each day the strategy signal function returns a target exposure
 *  ∈ [−1, 1] and the engine rebalances the portfolio accordingly.
 *
 *  Outputs (written into the strategy_config_t):
 *    • annual_profit  — CAGR  (compounded annual growth rate)
 *    • sharpe_ratio   — annualized Sharpe (μ / σ, risk-free = 0)
 *
 *  The equity_curve array (passed in by the caller) records the
 *  portfolio value at each time step so it can be plotted later.
 *
 *  Position model:
 *    - signal > 0  → allocate that fraction of net worth to a long
 *                     position; close any existing short first.
 *    - signal < 0  → allocate that fraction of net worth to a short
 *                     position; close any existing long first.
 *    - signal = 0  → no change (hold existing position).
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
 * Function-pointer table for strategy dispatch.
 * Indexed by run_config_t.strategy_index, which is determined from
 * the strategy's position in strategies.json.
 */
float (*get_signal[])(unsigned day,
                      strategy_config_t * strategy_config,
                      float * prices) = {
    signal_SMA_crossover,
    signal_RSI,
};


void backtest(run_config_t run,
              strategy_config_t * strategy_config,
              float * prices,
              float * equity_curve)
{
    float cash         = BUDGET;
    float assets_owned = 0.0f;   // units of the asset held long
    float asset_loans  = 0.0f;   // units borrowed (short position)
    float networth = 0.0f;

    /* reset strategy-local storage before this run */
    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy_config->storage[i] = NAN;
    }

    unsigned start = run.start;
    unsigned end   = run.end;

    /*
     * known_prices: a lookahead-safe copy of the price series.
     * Prices before 'start' are real historical data; prices from
     * 'start' onward are initially set to NAN so the strategy
     * cannot peek into the future.  Each day we reveal one more
     * price as it becomes "known".
     */
    float known_prices[end];
    for(unsigned i = 0; i < end; i++){
        if(i < start){
            known_prices[i] = prices[i];
        } else{
            known_prices[i] = NAN;
        }
    }

    /* ---- main simulation loop, one iteration per trading day ---- */
    for(unsigned i = start; i < end; i++){

        known_prices[i] = prices[i];   // reveal today's price

        /* mark-to-market net worth */
        networth = (assets_owned - asset_loans) * known_prices[i] + cash;
        equity_curve[i - start] = networth;

        if(networth <= 0){
            /* Portfolio wiped out — zero the remaining curve and stop. */
            networth = 0;
            for(unsigned j = i; j < end; j++){
                equity_curve[j - start] = 0;
            }
            break;
        }

        /* query the strategy for today's desired exposure */
        float signal = get_signal[run.strategy_index](
                            i, strategy_config, known_prices);

        /* target position size = signal × net worth, in asset units */
        float desired_investment = signal * networth / known_prices[i];

        float traded_volume = 0.0f;

        if(desired_investment > 0){
            /* ---- enter / adjust long position ---- */
            float cover_vol = asset_loans * known_prices[i];
            cash  -= cover_vol;                        // cover shorts
            traded_volume += fabsf(cover_vol);
            asset_loans = 0;

            float buy_vol = (desired_investment - assets_owned)
                            * known_prices[i];
            cash  -= buy_vol;                          // buy the delta
            traded_volume += fabsf(buy_vol);
            assets_owned = desired_investment;

        } else if(desired_investment < 0){
            /* ---- enter / adjust short position ---- */
            desired_investment = fabsf(desired_investment);

            float sell_vol = assets_owned * known_prices[i];
            cash  += sell_vol;                         // sell longs
            traded_volume += fabsf(sell_vol);
            assets_owned = 0;

            float short_vol = (desired_investment - asset_loans)
                              * known_prices[i];
            cash  += short_vol;                        // borrow & sell
            traded_volume += fabsf(short_vol);
            asset_loans = desired_investment;
        }

        /* deduct transaction cost */
        cash -= traded_volume * run.transaction_cost;
    }  /* end of daily loop */

    /*
     * Recompute final net worth: the value captured at the top of
     * the last iteration was pre-trade.  We now mark to market
     * using the last day's price and the post-trade cash/positions.
     * If the portfolio wiped out (networth == 0) we skip this step.
     */
    if (networth > 0.0f) {
        networth = (assets_owned - asset_loans) * prices[end - 1] + cash;
    }

    unsigned n_days   = end - start;
    float total_return = (networth - BUDGET) / BUDGET;

    /* ---- annualized profit (CAGR) ---- */
    if (networth > 0.0f) {
        strategy_config->performance.annual_profit =
            powf(1.0f + total_return,
                 ((float)run.trading_days / n_days)) - 1.0f;
    } else {
        strategy_config->performance.annual_profit = -1.0f;
    }
    strategy_config->performance.total_return = total_return;

    /* ---- daily-return-based statistics ---- */
    unsigned number_of_returns = n_days - 1;
    float daily_returns[number_of_returns];

    float sum_daily_returns   = 0.0f;
    float sum_sq_returns      = 0.0f;   // for volatility
    float sum_sq_downside     = 0.0f;   // for Sortino
    float peak                = equity_curve[0];
    float max_dd              = 0.0f;

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

        daily_returns[i] = (equity_curve[i + 1] - equity_curve[i])
                           / equity_curve[i];
        float dr = daily_returns[i];

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
        float dd = (equity_curve[number_of_returns] - peak)
                   / peak;
        if (dd < max_dd)
            max_dd = dd;
    }
    strategy_config->performance.max_drawdown = max_dd;

    /* guard: flat equity curve or single data point */
    if (number_of_returns == 0 || sum_daily_returns == 0.0f) {
        strategy_config->performance.sharpe_ratio  = 0.0f;
        strategy_config->performance.sortino_ratio = 0.0f;
        strategy_config->performance.volatility    = 0.0f;
        strategy_config->performance.calmar_ratio  = 0.0f;
        return;
    }

    float mean_daily = sum_daily_returns / number_of_returns;

    /* variance = E[r²] − E[r]²  (single-pass) */
    float variance = sum_sq_returns / number_of_returns
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
    } else {
        strategy_config->performance.sharpe_ratio = 0.0f;
    }

    /* Sortino ratio (downside deviation) */
    float downside_variance = sum_sq_downside / number_of_returns;
    float downside_dev = sqrtf(downside_variance);
    if (downside_dev > 0.0f) {
        strategy_config->performance.sortino_ratio =
            (mean_daily - RISK_FREE_RATE) / downside_dev * ann_factor;
    } else {
        strategy_config->performance.sortino_ratio = 0.0f;
    }

    /* Calmar ratio */
    if (max_dd < 0.0f) {
        strategy_config->performance.calmar_ratio =
            strategy_config->performance.annual_profit
            / fabsf(max_dd);
    } else {
        strategy_config->performance.calmar_ratio = 0.0f;
    }
}
