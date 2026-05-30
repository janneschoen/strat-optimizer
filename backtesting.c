#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BUDGET 10000
#define RISK_FREE_RATE 0

float (*get_signal[])(unsigned day, strategy_config_t * strategy_config, float * prices) = {
    signal_SMA_crossover,
    signal_RSI,
};

// Backtesting logic

void backtest(run_config_t run, strategy_config_t * strategy_config, float * prices, float * equity_curve){
    float cash = BUDGET, assets_owned = 0, asset_loans = 0;
    float networth;
    unsigned start = run.start;
    unsigned end = run.end;

    for(unsigned i = start; i < end; i++){
        float known_prices[end]; // prices get_signal function is allowed to know at this time

        for(unsigned j = 0; j < end; j++){
            if(j <= i){
                known_prices[j] = prices[j];
            } else{
                known_prices[j] = NAN;
            }
        }

        networth = (assets_owned - asset_loans) * known_prices[i] + cash;
        equity_curve[i - start] = networth;

        if(networth <= 0){
            networth = 0;
            for(unsigned j = i; j < end; j++){
                equity_curve[j] = 0;
            }
            break;
        }

        float signal = get_signal[run.strategy_index](i, strategy_config, prices);

        float desired_investment = signal * networth / known_prices[i];

        if(desired_investment > 0){ // entering long position
            cash -= (asset_loans * known_prices[i]); // covering shorts
            asset_loans = 0;
            cash -= (desired_investment - assets_owned) * known_prices[i]; // buying 
            assets_owned = desired_investment;

        } else if(desired_investment < 0){ // entering short position
            desired_investment = fabs(desired_investment);
            cash += (assets_owned * known_prices[i]); // closing longs
            assets_owned = 0;
            cash += (desired_investment - asset_loans) * known_prices[i]; // selling
            asset_loans = desired_investment;
        }
    }


    // Annualize profit

    float profit = (networth - BUDGET) / BUDGET;

    strategy_config->performance.annual_profit = pow((1 + profit), ((float)run.trading_days / (end-start))) - 1;

    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy_config->storage[i] = NAN;
    }


    // Calculate sharpe ratio

    unsigned number_of_returns = end - start - 1;

    float daily_returns[number_of_returns];

    float sum_daily_returns = 0;
    for(unsigned i = 0; i < number_of_returns; i++){
        daily_returns[i] = (equity_curve[i + 1] - equity_curve[i]) / equity_curve[i];
        sum_daily_returns += daily_returns[i];
    }
    float mean_daily_return = sum_daily_returns / number_of_returns;

    float sum_squared_deviations = 0;
    for(unsigned i = 0; i < number_of_returns; i++){
        sum_squared_deviations += pow(daily_returns[i] - mean_daily_return, 2);
    }

    float standard_deviation = pow(sum_squared_deviations / number_of_returns, 0.5);

    float sharpe_ratio = (mean_daily_return - RISK_FREE_RATE) / standard_deviation * pow(run.trading_days, 0.5);
    strategy_config->performance.sharpe_ratio = sharpe_ratio;
}