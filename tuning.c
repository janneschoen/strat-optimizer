#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void genStrats(unsigned stratTypeID, unsigned param, strat_t * strategies, unsigned numStrats, strat_t * strategy, unsigned * stratsMade){
    if(param == stratTypes[stratTypeID].numParams){
        if(stratTypes[stratTypeID].validStrat(strategy)){
            for(unsigned i = 0; i < NUM_PERF_TYPES; i++){
                strategy->performance[i] = NAN;
            }
            strategies[*stratsMade] = * strategy; 
            (*stratsMade) ++;
        }
    } else{
        for(float i = stratTypes[stratTypeID].minParams[param]; i <= strategy->params[param]; i+=config.gridIntv[param]){
            strat_t stratCpy = *strategy;
            stratCpy.params[param] = i;
            genStrats(stratTypeID, param + 1, strategies, numStrats, &stratCpy, stratsMade);
        }
    }
}

strat_t perfByParams(unsigned stratTypeID, strat_t * strategies, strat_t strategy, unsigned numStrats){
    unsigned numParams = stratTypes[stratTypeID].numParams;
    unsigned index = 0;
    for(unsigned i = 0; i < numStrats; i++){
        unsigned matching = 0;
        for(unsigned j = 0; j < numParams; j++){
            if(strategies[i].params[j] == strategy.params[j]){
                matching ++;
                if(matching == numParams){
                    return strategies[index];
                }
            } else{
                break;
            }
        }
    }
    printf("ERROR: Strategy not found.\n");
    exit(1);
}

strat_t findBestStrat(strat_t * strategies, unsigned numStrats){
    strat_t bestStrat = strategies[0];
    for(unsigned i = 1; i < numStrats; i++){
        if(strategies[i].performance[config.goal] > bestStrat.performance[config.goal]){
            bestStrat = strategies[i];
        }
    }
    return bestStrat;
}

#define TOLERANCE 1e-4

strat_t findOptimalStrat(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
    unsigned numParams = stratTypes[stratTypeID].numParams;
    float split[numParams][3];

    float refinedParams[numParams];
    for(unsigned i = 0; i < numParams; i++){
        refinedParams[i] = NAN;
    }
    unsigned paramsRefined = 0;
    unsigned hDim = 0;

    for(unsigned i = 0; i < numParams; i++){
        split[i][0] = stratTypes[stratTypeID].minParams[i];
        split[i][2] = strategies[numStrats-1].params[i];
        split[i][1] = (split[i][0] + split[i][2]) / 2;
    }

    while(paramsRefined < numParams){
        while(hDim == numParams || !isnan(refinedParams[hDim])){
            if(hDim == numParams){
                hDim = 0;
            }
            if(!isnan(refinedParams[hDim])){
                hDim ++;
            }
        }

        float lowerSum = 0, upperSum = 0;
        for(unsigned i = 0; i < numStrats; i++){
            unsigned paramsInLowerHalf = 0;
            unsigned paramsInUpperHalf = 0;
            for(unsigned j = 0; j < numParams; j++){
                float param = strategies[i].params[j];
                if(param >= split[j][0] && param <= split[j][j==hDim?1:2]){
                    paramsInLowerHalf ++;
                }
                if(param >= split[j][j==hDim?1:0] && param <= split[j][2]){
                    paramsInUpperHalf ++;
                }
            }
            if(paramsInLowerHalf == numParams){
                lowerSum += strategies[i].performance[config.goal];
            }
            if(paramsInUpperHalf == numParams){
                upperSum += strategies[i].performance[config.goal];
            }
        }
        printf("Lowerhalf: %.2f\n", lowerSum);
        printf("Upperhalf: %.2f\n", upperSum);
        printf("\n");

        if(lowerSum > upperSum){
            split[hDim][2] = split[hDim][1];
        } else{
            split[hDim][0] = split[hDim][1];
        }
        split[hDim][1] = (split[hDim][0] + split[hDim][2]) / 2;

        if(fabs(split[hDim][0] - split[hDim][2]) < TOLERANCE){
            paramsRefined ++;
            refinedParams[hDim] = (split[hDim][0]+split[hDim][1]+split[hDim][2]) / 3;
        }

        hDim++;
    }

    strat_t optimalStrat;
    for(unsigned i = 0; i < numParams; i++){
        optimalStrat.params[i] = refinedParams[i];
    }
    optimalStrat.performance[config.goal] = NAN;
    return optimalStrat;
}