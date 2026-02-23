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

float avgPrice(float * prices, unsigned from, unsigned to){
    float priceSum = 0;
    for(unsigned i = from; i < to; i++){
        priceSum += prices[i];
    }
    return priceSum / (to-from);
}

float getSignal0(unsigned day, strat_t * strategy, float * prices, float networth, float cash){
    unsigned fastL = strategy->params[0];
    unsigned slowL = strategy->params[1];
    unsigned entryType = strategy->params[3];

    float fastSMA = avgPrice(prices, day-fastL, day);
    float slowSMA = avgPrice(prices, day-slowL, day);

    float desiredInv = networth * (1.0 - strategy->params[2]);

    if(entryType == 1 && networth == cash){
        float oldFastSMA = avgPrice(prices, day-fastL-1, day-1);
        float oldSlowSMA = avgPrice(prices, day-slowL-1, day-1);
        if(fastSMA > slowSMA && oldFastSMA < oldSlowSMA){
            return desiredInv;
        } else if (fastSMA < slowSMA && oldFastSMA > oldSlowSMA){
            return -desiredInv;
        } else{
            return 0;
        }
    }

    if(fastSMA > slowSMA){
        return desiredInv;
    } else{
        return -desiredInv;
    }
}
