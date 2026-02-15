#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float getSignal0(unsigned day, strat_t * strategy, float * prices);
bool validStrat0(strat_t * strategy);

const stratType_t stratTypes[NUM_STRAT_TYPES] ={
    {"Simple Moving Average", 3, {"SMA Length", "LMA Length", "Sensitivity"}, {1,1,0}, getSignal0, validStrat0},
};

const char * perfTypes[] = {
    "Annualized Profit",
    "Sharpe Ratio"
};

bool validStrat0(strat_t * strategy){
    if(strategy->params[0] < strategy->params[1]){
        return 1;
    }
    return 0;
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
float getSignal0(unsigned day, strat_t * strategy, float * prices){
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

    float diff = (sma-lma)/lma;
    float sigTol = (float)strategy->params[2]/100;

    if(sigTol == 0){
        return diff > 0 ? 1 : -1;
    }

    float position = diff / sigTol;

    if(position > 1){
        position = 1;
    } else if(position < -1){
        position = -1;
    }
    return(position);
}