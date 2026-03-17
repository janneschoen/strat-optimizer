#include "common.h"
#include <stdio.h>

// Strategy Type: Simple Moving Average Crossover

float avgPrice(float * prices, unsigned from, unsigned to){
    float priceSum = 0;
    for(unsigned i = from; i < to; i++){
        priceSum += prices[i];
    }
    return priceSum / (to-from);
}

float SMA_crossover_signal(unsigned day, strat_t * strategy, float * prices){

    // storage[0] stores yesterdays crossover state

    unsigned fastL = strategy->params[0];
    unsigned slowL = strategy->params[1];
    float fastSMA = avgPrice(prices, day-fastL, day);
    float slowSMA = avgPrice(prices, day-slowL, day);

    int signal = fastSMA > slowSMA ? 1 : -1;
    int lastSignal = strategy->storage[0];

    float invSize = strategy->params[2];
    if(signal != lastSignal){
        strategy->storage[0] = signal;
        return invSize * signal;
    }
    return 0;
}
