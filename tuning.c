#include "common.h"
#include <stdio.h>
#include <stdlib.h>


void visualise(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
    FILE * file;
    file = fopen(STRAT_FILE, "w");
    for(unsigned i = 0; i < numStrats; i++){
        for(unsigned j = 0; j < stratTypes[stratTypeID].numParams; j++){
            fprintf(file, "%u ", strategies[i].params[j]);
        }
        fprintf(file, "%f\n", strategies[i].performance);
    }
    fclose(file);
    char command[50];
    sprintf(command, "python %s %u %u", PLOTTING_PY, numStrats, stratTypes[stratTypeID].numParams);
    system(command);
}

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

void testStrats(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, float * prices, unsigned priceAmount, unsigned start){
    for(unsigned i = 0; i < numStrats; i++){
        strategies[i].performance = backtest(stratTypeID, &strategies[i], prices, priceAmount, start);
        if(i % 2500 == 0){
            loadingBar(i, numStrats);
        }
    }
}

strat_t findBestStrat(strat_t * strategies, unsigned numStrats){
    strat_t bestStrat = strategies[0];
    for(unsigned i = 1; i < numStrats; i++){
        if(strategies[i].performance > bestStrat.performance){
            bestStrat = strategies[i];
        }
    }
    return bestStrat;
}