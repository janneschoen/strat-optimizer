#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


const char * perfTypes[] = {
    "Annualized Profit",
    "Sharpe Ratio"
};

unsigned getLookback(unsigned stratTypeID, strat_t * strategy){
    unsigned lookback;
    switch(stratTypeID){
        case 0:
            lookback = strategy->params[0] > strategy->params[1] ?
            strategy->params[0] : strategy->params[1];
            return lookback;
        case 1:
            lookback = getLookback(0, strategy) + strategy->params[3];
            return lookback;
        default:
            exit(1);
    }
}

bool validStrat0(strat_t * strategy){
    if(strategy->params[0] >= strategy->params[1]){
        return 0;
    }
    if(strategy->params[2] > 100){
        return 0;
    }
    return 1;
}

// strategy 0 : Simple Moving Average
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

bool validStrat1(strat_t * strategy){
    if(!validStrat0(strategy)){
        return 0;
    }
    return 1;
}

float getSignal1(unsigned day, strat_t * strategy, float * prices, float networth){
    strat_t trainedStrat;
    for(unsigned i = 0; i < stratTypes[1].numParams; i++){
        trainedStrat.params[i] = strategy->storage[i];
    }

    if((day - getLookback(1, strategy)) % strategy->params[4] == 0){
        unsigned numStrats = 1;
        for(unsigned i = 0; i < stratTypes[0].numParams; i++){
            if(stratTypes[0].minParams[i] == 0){
                numStrats *= strategy->params[i] + 1;
            } else{
                numStrats *= strategy->params[i] / GRID_INTERVAL;
            }
        }

        strat_t * strategies = (strat_t *)malloc(numStrats * sizeof(strat_t));

        unsigned stratsMade = 0;
        genStrats(0, 0, strategies, numStrats, strategy, &stratsMade);

        numStrats = stratsMade;

        for(unsigned i = 0; i < numStrats; i++){
            unsigned start = day - strategy->params[3];
            backtest(0, &strategies[i], prices, start, day);
        }

        trainedStrat = findBestStrat(strategies, numStrats, 0);
        
        for(unsigned i = 0; i < stratTypes[0].numParams; i++){
            strategy->storage[i] = trainedStrat.params[i];
        }

        free(strategies);
    }

    return getSignal0(day, &trainedStrat, prices, networth);
}

const stratType_t stratTypes[NUM_STRAT_TYPES] ={
    {"Simple Moving Average", 3, {"SMA len", "LMA len", "Reserves"}, {1,1,0}, getSignal0, validStrat0},
    {"Dynamic SMA", 5, {"Max SMA len", "Max LMA len", "Reserves max", "Training size", "Training freq"}, {1,1,0,2,1}, getSignal1, validStrat1}
};