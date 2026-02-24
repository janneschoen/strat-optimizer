#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RISK_FREE_RATE 0.02
#define TRADING_FEE 0.002

void backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned start, unsigned end){
    if(!stratTypes[stratTypeID].validStrat(strategy)){
        printf("WARNING: invalid strat given to backtest.\n");
    }
    float cash = BUDGET, assetsOwned = 0, assetLoans = 0;
    float networth = cash;
    int pos = 0;

    float networthValues[end-start];
    for(unsigned i = 0; i < end-start; i++){
        networthValues[i] = 0;
    }

    for(unsigned i = start; i < end; i++){
        networth = (assetsOwned - assetLoans) * prices[i] + cash;

        if(networth <= 0){
            networth = 0;
            break;
        }

        int signal = stratTypes[stratTypeID].getSignal(i, strategy, prices, pos);

        float desPosSize = networth / prices[i];
        if(signal == 1){
            cash -= assetLoans * prices[i]; // close all shorts
            assetLoans = 0;

            cash += (assetsOwned - desPosSize) * prices[i];
            assetsOwned = desPosSize;
            pos = 1;

        } else if(signal == -1){
            cash += assetsOwned * prices[i]; // close all longs
            assetsOwned = 0;

            cash += (desPosSize - assetLoans) * prices[i];
            assetLoans = desPosSize;
            pos = -1;
        }
        networthValues[i-start] = networth;
    }

    float perf = (networth - BUDGET) / BUDGET;
    float yearLen = config.fullYear ? 365 : 252;

    strategy->performance[0] = pow(1 + perf, yearLen / (end-start)) - 1;

    if(config.singleTest && config.visuals[0]){
        FILE * file = fopen(CHART_FILE, "w");
        for(unsigned i = 0; i < end-start; i++){
            fprintf(file, "%f\n", networthValues[i]);
        }
        fclose(file);
    }

    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy->storage[i] = NAN;
    }
}

void getPrices(char * ticker, unsigned priceAmount, float * prices){

    char command[50];
    
    sprintf(command, "python %s %s %u %u", GETPRICES_PY, ticker, priceAmount, config.fullYear);
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