#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RISK_FREE_RATE 0.02

float backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount, unsigned start){
    float cash = 100;
    float assets = 0;
    float networth, posSize, prevNetworth;

    unsigned numDailyReturns = priceAmount-start-1;
    double dailyReturns[numDailyReturns];
    for(unsigned i = 0; i < numDailyReturns; i++){
        dailyReturns[i] = NAN;
    }

    for(unsigned i = start; i < priceAmount; i++){
        networth = cash + assets * prices[i];
        posSize = stratTypes[stratTypeID].getSignal(i, strategy, prices);
        assets = (posSize * networth) / prices[i];
        cash = networth - posSize * networth;
        
        if(i > start){
            dailyReturns[i-start-1] = (networth-prevNetworth) / prevNetworth;
        }
        prevNetworth = networth;
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

    double stdDeviation = pow(variance, 0.5);

    double profit = (networth - 100) / 100;

    double period = (float)(priceAmount-start) / 252;

    float baseCase = pow((1 + RISK_FREE_RATE), period) - 1;

    float sharpeRatio = (profit - baseCase) / stdDeviation;

    //double ppy = pow((1 + profit), 1.0/period) - 1;

    return (sharpeRatio);
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