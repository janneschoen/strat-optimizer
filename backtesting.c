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
    float networth;

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
                            i, strategy_config, prices);

        /* target position size = signal × net worth, in asset units */
        float desired_investment = signal * networth / known_prices[i];

        if(desired_investment > 0){
            /* ---- enter / adjust long position ---- */
            cash  -= (asset_loans * known_prices[i]);  // cover shorts
            asset_loans = 0;
            cash  -= (desired_investment - assets_owned)
                     * known_prices[i];                 // buy the delta
            assets_owned = desired_investment;

        } else if(desired_investment < 0){
            /* ---- enter / adjust short position ---- */
            desired_investment = fabs(desired_investment);
            cash  += (assets_owned * known_prices[i]); // sell longs
            assets_owned = 0;
            cash  += (desired_investment - asset_loans)
                     * known_prices[i];                 // borrow & sell
            asset_loans = desired_investment;
        }
    }  /* end of daily loop */

    /* ---- annualized profit (CAGR) ---- */
    float profit = (networth - BUDGET) / BUDGET;
    strategy_config->performance.annual_profit =
        powf(1.0f + profit,
             ((float)run.trading_days / (end - start))) - 1.0f;

    /* reset strategy-local storage for the next combination */
    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy_config->storage[i] = NAN;
    }

    /* ---- annualized Sharpe ratio ---- */
    unsigned number_of_returns = end - start - 1;
    float daily_returns[number_of_returns];

    float sum_daily_returns = 0.0f;
    for(unsigned i = 0; i < number_of_returns; i++){
        daily_returns[i] = (equity_curve[i + 1] - equity_curve[i])
                           / equity_curve[i];
        sum_daily_returns += daily_returns[i];
    }
    float mean_daily_return = sum_daily_returns / number_of_returns;

    float sum_squared_deviations = 0.0f;
    for(unsigned i = 0; i < number_of_returns; i++){
        float dev = daily_returns[i] - mean_daily_return;
        sum_squared_deviations += dev * dev;
    }
    float std_daily = sqrtf(sum_squared_deviations / number_of_returns);

    /* annualize: Sharpe = μ_daily / σ_daily × √trading_days         */
    strategy_config->performance.sharpe_ratio =
        (mean_daily_return - RISK_FREE_RATE)
        / std_daily
        * sqrtf((float)run.trading_days);
}
