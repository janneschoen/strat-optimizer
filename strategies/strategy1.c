#include "common.h"
#include <stdlib.h>

// STRATEGY TYPE 1: dynamic simple moving average crossover

bool validStrat1(strat_t * strategy){
    if(!validStrat0(strategy)){
        return 0;
    }
    return 1;
}

float getSignal1(unsigned day, strat_t * strategy, float * prices, float networth, float cash){
    strat_t trainedStrat;
    for(unsigned i = 0; i < stratTypes[1].numParams; i++){
        trainedStrat.params[i] = strategy->storage[i];
    }

    if((day - getLookback(1, strategy)) % (unsigned)strategy->params[4] == 0){
        unsigned numStrats = 1;
        for(unsigned i = 0; i < stratTypes[0].numParams; i++){
            if(stratTypes[0].minParams[i] == 0){
                numStrats *= (strategy->params[i] + 1) / config.gridIntv[i];
            } else{
                numStrats *= strategy->params[i] / config.gridIntv[i];
            }
        }

        strat_t * trainingStrats = (strat_t *)malloc(numStrats * sizeof(strat_t));

        unsigned stratsMade = 0;
        genStrats(0, 0, trainingStrats, numStrats, strategy, &stratsMade);

        numStrats = stratsMade;

        for(unsigned i = 0; i < numStrats; i++){
            unsigned start = day - strategy->params[3];
            backtest(0, &trainingStrats[i], prices, start, day);
        }

        trainedStrat = findBestStrat(trainingStrats, numStrats);
        
        for(unsigned i = 0; i < stratTypes[0].numParams; i++){
            strategy->storage[i] = trainedStrat.params[i];
        }

        free(trainingStrats);
    }

    return getSignal0(day, &trainedStrat, prices, networth, cash);
}