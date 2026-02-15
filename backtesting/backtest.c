#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RISK_FREE_RATE 0.02
#define TRADING_FEE 0.001
#define BUDGET 600

void backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount, unsigned start, execMode_t * config){

    unsigned liquidation = priceAmount - start;
    unsigned numDailyReturns = liquidation - 1;
    double dailyReturns[numDailyReturns];
    float networthValues[liquidation];

    for(unsigned i = 0; i < liquidation; i++){
        networthValues[i] = NAN;
        if(i < numDailyReturns){
            dailyReturns[i] = NAN;
        }
    }

    float cash = BUDGET, assetsOwned = 0, assetLoans = 0;
    float networth = cash;
    float position;

    for(unsigned i = start; i < priceAmount; i++){
        networth = (assetsOwned - assetLoans) * prices[i] + cash;

        position = stratTypes[stratTypeID].getSignal(i, strategy, prices);

        if(position > 0){
            // buying back borrowed assets (closing shorts)
            cash -= assetLoans * prices[i];
            assetLoans = 0;

            // buying or selling assets
            float desiredAssets = (position * networth) / prices[i];
            cash += (assetsOwned - desiredAssets) * prices[i];
            assetsOwned = desiredAssets;

        } else if(position < 0){
            // selling all assets (closing longs)
            cash += assetsOwned * prices[i];
            assetsOwned = 0;

            // borrowing or giving back assets
            float desiredAssetLoans = (-1*position * networth) / prices[i];
            cash += (desiredAssetLoans - assetLoans) * prices[i];
            assetLoans = desiredAssetLoans;
        }

        if(networth <= 0){
            networth = 0;
            networthValues[i-start] = 0;
            liquidation = i-start;
            break;
        }

        networthValues[i-start] = networth;
    }

    numDailyReturns = liquidation - 1;

    if(config->singleTest){
        FILE * file = fopen(CHART_FILE, "w");
        for(unsigned i = 0; i < liquidation; i++){
            fprintf(file, "%f\n", networthValues[i]);
        }
        fclose(file);
    }

    unsigned tradingDays = config->fullYear ? 365 : 252;
    double period = tradingDays / (float)(priceAmount-start);
    double annProfit = pow(1 + ((networth - BUDGET) / BUDGET), period) - 1;

    for(unsigned i = 0; i < numDailyReturns; i++){
        dailyReturns[i] = (networthValues[i+1]-networthValues[i]) / networthValues[i];
    }

    double meanDailyReturn = 0;
    for(unsigned i = 0; i < numDailyReturns; i++){
        meanDailyReturn += dailyReturns[i];
    }
    meanDailyReturn /= numDailyReturns;

    double variance = 0;
    for(unsigned i = 0; i < numDailyReturns; i++){
        variance += pow((dailyReturns[i] - meanDailyReturn), 2);
    }
    variance /= numDailyReturns;

    double stdDeviation = pow(variance, 0.5) * pow(tradingDays, 0.5);

    strategy->performance[0] = annProfit;
    
    if(annProfit == 0){
        strategy->performance[1] = 0;
    } else{
        strategy->performance[1] = (annProfit - RISK_FREE_RATE) / stdDeviation;
    }
}

void getPrices(char * ticker, unsigned priceAmount, float * prices){

    char command[50];
    sprintf(command, "python %s %s %u", GETPRICES_PY, ticker, priceAmount);
    system(command);

    FILE * file;
    file = fopen(PRICES_FILE, "r");

    for(unsigned i = 0; i < priceAmount; i++){
        if (fscanf(file, "%f", &prices[i]) != 1) {
            printf("ERROR: Prices could not be properly downloaded.\n");
            fclose(file);
            exit(1);
        }
    }
    fclose(file);
}