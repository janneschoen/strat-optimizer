#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BUDGET 10000


float (*get_signal[])(unsigned day, strategy_config_t * strategy_config, float * prices) = {
    signal_SMA_crossover,
    signal_RSI,
};

// Backtesting logic

void backtest(unsigned strategy_index, strategy_config_t * strategy_config, float * prices, unsigned start, unsigned end, float * equity_curve){
    float cash = BUDGET, assets_owned = 0, asset_loans = 0;
    float networth;

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
            break;
        }

        float signal = get_signal[strategy_index](i, strategy_config, prices);

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

    float profit = (networth - BUDGET) / BUDGET;

    strategy_config->performance[0] = profit;

    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy_config->storage[i] = NAN;
    }
}