#include "common.h"
#include <stdio.h>
#include <stdlib.h>

void genStrats(unsigned stratTypeID, unsigned param, strat_t * strategies, unsigned numStrats, strat_t * strategy, unsigned * stratsMade){
    if(param == stratTypes[stratTypeID].numParams){
        strategies[*stratsMade] = * strategy; 
        (*stratsMade) ++;
    } else{
        for(unsigned i = 1; i <= strategy->params[param]; i++){
            strat_t stratCpy = *strategy;
            stratCpy.params[param] = i;
            genStrats(stratTypeID, param + 1, strategies, numStrats, &stratCpy, stratsMade);
        }
    }
}
void testStrats(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, float * prices, unsigned priceAmount){
    printf("Testing %u strategies... \n", numStrats);
    for(unsigned i = 0; i < numStrats; i++){
        strategies[i].performance = backtest(stratTypeID, &strategies[i], prices, priceAmount);
        if((i+1) % 20000 == 0){
            float progress = (float)(i+1) / numStrats * 100;
            printf("\r%.1f%%", progress);
            fflush(stdout);
        }
    }
    printf("\r");
}

void findBestStrat(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
    strat_t bestStrat = strategies[0];
    for(unsigned i = 1; i < numStrats; i++){
        if(strategies[i].performance > bestStrat.performance){
            bestStrat = strategies[i];
        }
    }
    printf("Best test: [ ");
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        printf(" %u ", bestStrat.params[i]);
    }
    printf("] | Profit %.2f%% / year\n", bestStrat.performance * 100);
}