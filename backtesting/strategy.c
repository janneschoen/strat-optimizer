#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float getSignal0(unsigned day, strat_t * strategy, float * prices, float networth);
bool validStrat0(strat_t * strategy);

const stratType_t stratTypes[NUM_STRAT_TYPES] ={
    {"Simple Moving Average", 3, {"SMA Length", "LMA Length", "Allocation"}, {1,1,0}, getSignal0, validStrat0},
};

const char * perfTypes[] = {
    "Annualized Profit",
    "Sharpe Ratio"
};

bool validStrat0(strat_t * strategy){
    if(strategy->params[0] >= strategy->params[1]){
        return 0;
    }
    if(strategy->params[2] > 100){
        return 0;
    }
    return 1;
}


unsigned getLookback(unsigned stratTypeID, strat_t * strategy){
    switch(stratTypeID){
        case 0:
            return strategy->params[0] > strategy->params[1] ?
            strategy->params[0] : strategy->params[1];
        case 1:
            return getLookback(0, strategy) + strategy->params[3];
        default:
            exit(1);
    }
}

// stratType0 : Simple Moving Average
float getSignal0(unsigned day, strat_t * strategy, float * prices, float networth){
    float shortPriceSum = 0;
    for(unsigned i = 0; i < strategy->params[0]; i++){
        shortPriceSum += prices[day-i];
    }
    float sma = shortPriceSum / strategy->params[0];

    float longPriceSum = 0;
    for(unsigned i = 0; i < strategy->params[1]; i++){
        longPriceSum += prices[day-i];
    }
    float lma = longPriceSum / strategy->params[1];

    float allocation = networth * (1.0 - ((float)strategy->params[2] / 100.0));

    if(sma > lma){
        return allocation;
    } else{
        return -allocation;
    }
}