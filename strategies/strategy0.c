#include "common.h"
#include <math.h>
#include <stdio.h>

// STRATEGY TYPE 0: simple moving average crossover

bool validStrat0(strat_t * strategy){
    float p0 = strategy->params[0];
    float p1 = strategy->params[1];
    float p2 = strategy->params[2];
    if(p0 != (unsigned)p0 || p1 != (unsigned)p1){
        return 0;
    }
    if(p0 >= p1){
        return 0;
    }
    if(p2 > 100){
        return 0;
    }
    return 1;
}

float getSignal0(unsigned day, strat_t * strategy, float * prices, float networth, float cash){
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

    float desiredInv = networth * (1.0 - strategy->params[2]);

    if(networth == cash){
        if(fabs(sma-lma)/lma > strategy->params[3]){
            return 0;
        }
    }

    if(sma > lma){
        return desiredInv;
    } else{
        return -desiredInv;
    }
}
