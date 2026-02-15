#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RISK_FREE_RATE 0.02
#define TRADING_FEE 1 // trade republic: 1€ per trade
#define BUDGET 1200

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

    float cash = BUDGET, longs = 0, shorts = 0;
    float shortEntry, networth = cash;
    int position;

    for(unsigned i = start; i < priceAmount; i++){
        if(shorts > 0){
            networth = shorts * (2 * shortEntry - prices[i]);
        } else if(longs > 0){
            networth = longs * prices[i];
        } else{
            networth = cash;
        }

        position = stratTypes[stratTypeID].getSignal(i, strategy, prices);
        switch(position){
            case -1:
                if(shorts == 0){
                    shortEntry = prices[i];
                    shorts = (networth - TRADING_FEE)  / prices[i];
                    cash = 0, longs = 0;
                }
                break;
            case 1:
                if(longs == 0){
                    longs = (networth - TRADING_FEE) / prices[i];
                    cash = 0, shorts = 0;
                }
                break;
            case 0:
                if(longs != 0 || shorts != 0){
                    cash = networth - TRADING_FEE;
                    longs = 0, shorts = 0;
                }
                break;
            default:
                printf("ERROR.\n");
                exit(1);
        }

        if(shorts > 0){
            networth = shorts * (2 * shortEntry - prices[i]);
        } else if(longs > 0){
            networth = longs * prices[i];
        } else{
            networth = cash;
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