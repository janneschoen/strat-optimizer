#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RISK_FREE_RATE 0.02

float backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount, unsigned start){
    float cash = 100;
    float assets = 0;
    float networth, posSize;
    for(unsigned i = start; i < priceAmount; i++){
        networth = cash + assets * prices[i];
        posSize = stratTypes[stratTypeID].getSignal(i, strategy, prices);
        assets = (posSize * networth) / prices[i];
        cash = networth - posSize * networth;
    }
    double totalPerf = (networth - 100) / 100;
    double period = 252 / (float)(priceAmount-start);
    double ppy = pow((1 + totalPerf), period) - 1;

    return (ppy);
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