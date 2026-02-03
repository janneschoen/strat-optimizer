#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


void getPrices(char * ticker, unsigned priceAmount, float * prices){

    char command[50];
    sprintf(command, "python %s %s %u", GETPRICES_PY, ticker, priceAmount);
    system(command);

    FILE * file;
    file = fopen(PRICES_FILE, "r");
    unsigned zerosCounted = 0;
    for(unsigned i = 0; i < priceAmount; i++){
        fscanf(file, "%f", &prices[i]);
        if(prices[i] == 0){
            zerosCounted ++;
        }
    }
    if(zerosCounted == priceAmount){
        printf("ERROR: No valid prices.\n");
        exit(1);
    }
    fclose(file);
}

float backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount, unsigned start){
    float cash = 100;
    float assets = 0;
    float networth, posSize;
    for(unsigned i = start; i < priceAmount; i++){
        if(prices[i] > 0){
            networth = cash + assets * prices[i];
            posSize = stratTypes[stratTypeID].getSignal(i, strategy, prices);
            assets = (posSize * networth) / prices[i];
            cash = networth - posSize * networth;
        }
    }
    double totalPerf = (networth - 100) / 100;
    double period = 365 / (float)priceAmount;
    double ppy = pow((1 + totalPerf), period) - 1;

    return (ppy);
}