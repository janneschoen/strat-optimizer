#include "common.h"
#include <stdio.h>

// STRATEGY TYPE 0: Simple Moving Average Crossover

float avgPrice(float * prices, unsigned from, unsigned to){
    float priceSum = 0;
    for(unsigned i = from; i < to; i++){
        priceSum += prices[i];
    }
    return priceSum / (to-from);
}

int SMA_Crossover(unsigned day, strat_t * strategy, float * prices){

    // storage[0] stores yesterdays crossover state

    unsigned fastL = strategy->params[0];
    unsigned slowL = strategy->params[1];
    float fastSMA = avgPrice(prices, day-fastL, day);
    float slowSMA = avgPrice(prices, day-slowL, day);

    int fastAboveSlow = fastSMA > slowSMA ? 1 : -1;
    if(fastAboveSlow != strategy->storage[0]){
        strategy->storage[0] = fastAboveSlow;
        return fastAboveSlow;
    }
    return 0;
}
