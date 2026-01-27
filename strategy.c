#include "common.h"
#include <stdio.h>
#include <stdlib.h>

const stratType_t stratTypes[NUM_STRAT_TYPES] ={
    {"Simple Moving Average", 3, {"SMA Length", "LMA Length", "Sensitivity"}}
};


unsigned getLookback(unsigned stratTypeID, strat_t * strategy){
    switch(stratTypeID){
        case 0:
            return strategy->params[0] > strategy->params[1] ?
            strategy->params[0] : strategy->params[1];
        default:
            exit(1);
    }
}
float getPos(unsigned stratTypeID, strat_t * strategy, unsigned day, float * prices){
    switch(stratTypeID){
        case 0:
            return genSignal0(day, strategy, prices);
        default:
            exit(1);
    }
}

// stratType0 : Simple Moving Average
float genSignal0(unsigned day, strat_t * strategy, float * prices){
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
        // 20% overshoot, 40% tol = 50% posSize (0.2 / 0.4 = 0.5)
        float posTolRatio = ((sma-lma) / lma) / ((float)strategy->params[2] / 100);
        
        if(posTolRatio > 1){
            return 1;
        } else{
            return(posTolRatio);
        }
    } else{
        return 0;
    }
}