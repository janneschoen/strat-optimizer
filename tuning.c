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
        for(unsigned i = stratTypes[stratTypeID].minParams[param]; i <= strategy->params[param]; i++){
            strat_t stratCpy = *strategy;
            stratCpy.params[param] = i;
            genStrats(stratTypeID, param + 1, strategies, numStrats, &stratCpy, stratsMade);
        }
    }
}

void testStrats(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, float * prices, unsigned priceAmount, unsigned start, unsigned fullYear){
    for(unsigned i = 0; i < numStrats; i++){
        strategies[i].performance = backtest(stratTypeID, &strategies[i], prices, priceAmount, start, 0, fullYear);
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

strat_t findOptimalStrat(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
    unsigned numParams = stratTypes[stratTypeID].numParams;
    unsigned maxParams[numParams];
    for(unsigned i = 0; i < numParams; i++){
        maxParams[i] = strategies[numStrats-1].params[i];
    }
    unsigned optimalParams[numParams];
    for(unsigned i = 0; i < numParams; i++){
        unsigned lowerHalf[2] = {stratTypes[stratTypeID].minParams[i], maxParams[i]/2};
        unsigned upperHalf[2] = {lowerHalf[1], maxParams[i]};

        while(!(lowerHalf[0] == lowerHalf[1] || upperHalf[0] == upperHalf[1])){

            float lowerSum = 0, upperSum = 0;

            for(unsigned j = 0; j < numStrats; j++){
                unsigned param = strategies[j].params[i];
                if(param >= lowerHalf[0] && param < lowerHalf[1]){
                    lowerSum += strategies[j].performance;
                } else if(param >= upperHalf[0] && param < upperHalf[1]){
                    upperSum += strategies[j].performance;
                }
            }

            if(lowerSum > upperSum){
                lowerHalf[1] = (lowerHalf[1] + lowerHalf[0]) / 2;
                upperHalf[1] = upperHalf[0];
                upperHalf[0] = lowerHalf[1];
            } else{
                upperHalf[0] = (upperHalf[0] + upperHalf[1]) / 2;
                lowerHalf[0] = lowerHalf[1];
                lowerHalf[1] = upperHalf[0];
            }

        }
        float perf1 = 0, perf2 = 0;
        for(unsigned j = 0; j < numStrats; j++){
            if(strategies[j].params[i] == upperHalf[0]){
                perf1 = strategies[j].performance;
            } else if(strategies[j].params[i] == upperHalf[1]){
                perf2 = strategies[j].performance;
            }
        }

        optimalParams[i] = perf1 > perf2 ? upperHalf[0] : upperHalf[1];

    }

    unsigned index = 0;
    for(unsigned i = 0; i < numStrats; i++){
        unsigned matching = 0;
        for(unsigned j = 0; j < numParams; j++){
            if(strategies[i].params[j] == optimalParams[j]){
                matching ++;
                if(matching == numParams){
                    index = i;
                }
            } else{
                break;
            }
        }
    }
    return strategies[index];
}