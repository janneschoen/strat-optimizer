#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define PRICES_FILE "prices.txt"

void getPrices(char * ticker, unsigned priceAmount, float * prices){
    unsigned download;
    printf("Local or new prices?\n");
    printf("0 - local\n");
    printf("1 - new\n");
    do{
        scanf("%u", &download);
    } while(download > 1);

    if(download){
        char command[50];
        printf("Downloading %u %s prices... \n", priceAmount, ticker);
        sprintf(command, "python getPrices.py %s %u", ticker, priceAmount);
        system(command);
    }

    FILE * file;
    file = fopen(PRICES_FILE, "r");
    for(unsigned i = 0; i < priceAmount; i++){
        fscanf(file, "%f", &prices[i]);
    }
    printf("Saved prices.\n");
    fclose(file);
}

float backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount){
    float cash = 100;
    float assets = 0;
    float networth, posSize;
    for(unsigned i = getLookback(stratTypeID, strategy); i < priceAmount; i++){
        if(prices[i] > 0){
            networth = cash + assets * prices[i];
            posSize = getPos(stratTypeID, strategy, i, prices);
            assets = (posSize * networth) / prices[i];
            cash = networth - posSize * networth;
        }
    }
    double totalPerf = (networth - 100) / 100;
    double period = 365 / (float)priceAmount;
    double ppy = pow((1 + totalPerf), period) - 1;

    return (ppy);
}