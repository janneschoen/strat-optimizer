#include "common.h"
#include <math.h>
#include <stdio.h>

// STRATEGY TYPE 0: simple moving average crossover

bool validStrat0(strat_t * strategy){
    float fastSMA = strategy->params[0];
    float slowSMA = strategy->params[1];
    float longTop = strategy->params[2];
    float shortBottom = strategy->params[3];

    if(fastSMA != (unsigned)fastSMA || slowSMA != (unsigned)slowSMA){
        return 0;
    }
    if(fastSMA >= slowSMA){
        return 0;
    }
    if(longTop <= 0){
        return 0;
    }
    if(shortBottom <= 0 || shortBottom >= 1){
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

int getSignal0(unsigned day, strat_t * strategy, float * prices, int pos){
    float longTop = strategy->params[2];
    float shortBottom = strategy->params[3];
    // storage[0] stores yesterdays crossover state
    // storage[1] stores the entry price

    if(pos == 1 && prices[day] >= strategy->storage[1] * (1.0 + longTop)){
        return -1;
    }
    if(pos == -1 && prices[day] <= strategy->storage[1] * (1.0 - shortBottom)){
        return 1;
    }

    unsigned fastL = strategy->params[0];
    unsigned slowL = strategy->params[1];
    float fastSMA = avgPrice(prices, day-fastL, day);
    float slowSMA = avgPrice(prices, day-slowL, day);

    int crossoverState = fastSMA > slowSMA ? 1 : -1;
    if(crossoverState != strategy->storage[0]){
        strategy->storage[0] = crossoverState;
        strategy->storage[1] = prices[day];
        return crossoverState;
    }
    return 0;
}
