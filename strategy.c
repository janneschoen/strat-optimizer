#include "common.h"
#include <stdio.h>
#include <stdlib.h>

float getSignal0(unsigned day, strat_t * strategy, float * prices);
float getSignal1(unsigned day, strat_t * strategy, float * prices);

const stratType_t stratTypes[NUM_STRAT_TYPES] ={
    {"Simple Moving Average", 3, {"SMA Length", "LMA Length", "Sensitivity"}, {1,1,0}, getSignal0},
};


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
    float longPriceSum = 0;
    for(unsigned i = 0; i < strategy->params[1]; i++){
        longPriceSum += prices[day-i];
    }
    float sma = shortPriceSum / strategy->params[0];
    float lma = longPriceSum / strategy->params[1];
    if(sma > lma){
        if(strategy->params[2] == 0){
            return 1;
        }

        float size = ((sma-lma) / lma) / ((float)strategy->params[2] / 100);
        
        if(size > 1){
            return 1;
        } else{
            return(size);
        }
    } else{
        return 0;
    }
}

/*float getSignal1(unsigned day, strat_t * strategy, float * prices){
    printf("Nothing yet.\n");
    exit(1);
}*/