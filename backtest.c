#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RISK_FREE_RATE 0.02

float backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount, unsigned start, unsigned testMode){
    float cash = 100;
    float assets = 0;
    float networth, posSize;

    unsigned numDailyReturns = priceAmount-start-1;
    double dailyReturns[numDailyReturns];

    float networthValues[priceAmount-start];
    for(unsigned i = 0; i < priceAmount-start; i++){
        networthValues[i] = NAN;
        dailyReturns[i] = NAN;
    }

    for(unsigned i = start; i < priceAmount; i++){
        networth = cash + assets * prices[i];
        posSize = stratTypes[stratTypeID].getSignal(i, strategy, prices);
        assets = (posSize * networth) / prices[i];
        cash = networth - posSize * networth;

        networthValues[i-start] = networth;
    }
    if(testMode){
        FILE * file = fopen(CHART_FILE, "w");
        for(unsigned i = 0; i < priceAmount-start; i++){
            fprintf(file, "%f\n", networthValues[i]);
        }
        fclose(file);
    }

    for(unsigned i = 0; i < priceAmount-start-1; i++){
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

    double stdDeviation = pow(variance, 0.5);

    double profit = (networth - 100) / 100;

    double period = (float)(priceAmount-start) / 252;

    float baseCase = pow((1 + RISK_FREE_RATE), period) - 1;
    float sharpeRatio = (profit - baseCase) / stdDeviation;

    if(stdDeviation == 0){
        return(0);
    }

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