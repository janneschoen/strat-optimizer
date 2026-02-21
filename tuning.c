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
                    index = i;
                }
            } else{
                break;
            }
        }
    }
    return strategies[index];
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

strat_t findOptimalStrat(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
    unsigned numParams = stratTypes[stratTypeID].numParams;
    unsigned maxParams[numParams];
    unsigned minParams[numParams];
    for(unsigned i = 0; i < numParams; i++){
        maxParams[i] = strategies[numStrats-1].params[i];
        minParams[i] = stratTypes[stratTypeID].minParams[i];
    }

    unsigned halvingParam = 0;

    unsigned half1[numParams][2];
    unsigned half2[numParams][2];

    for(unsigned i = 0; i < numParams; i++){
        half1[i][0] = minParams[i];
        half1[i][1] = i == halvingParam ? maxParams[i] / 2 : maxParams[i];
        half2[i][0] = i == halvingParam ? maxParams[i] / 2 + 1 : minParams[i];
        half2[i][1] = maxParams[i];
    }

    unsigned optimalParams[numParams];
    unsigned paramRefined[numParams];
    for(unsigned i = 0; i < numParams; i++){
        paramRefined[i] = 0;
    }
    unsigned refinedParams = 0;

    while(refinedParams < numParams){

        halvingParam ++;
        unsigned validParam = 0;
        while(!validParam){
            if(halvingParam < numParams && !paramRefined[halvingParam]){
                validParam = 1;
            }
            if(halvingParam >= numParams){
                halvingParam = 0;
            }
            if(paramRefined[halvingParam]){
                halvingParam ++;
            }
        }

        float value1 = 0, value2 = 0;
        for(unsigned i = 0; i < numStrats; i++){
            unsigned matching1 = 0, matching2 = 0;
            for(unsigned j = 0; j < numParams; j++){
                unsigned param = strategies[i].params[j];
                if(param >= half1[j][0] && param <= half1[j][1]){
                    matching1 ++;
                    if(matching1 == numParams){
                        value1 += strategies[i].performance[config.goal];
                    }
                }
                if(param >= half2[j][0] && param <= half2[j][1]){
                    matching2 ++;
                    if(matching2 == numParams){
                        value2 += strategies[i].performance[config.goal];
                    }
                }
            }
        }
        if(value1 > value2){
            for(unsigned i = 0; i < numParams; i++){
                if(i == halvingParam){
                    half2[i][1] = half1[i][1];
                    half1[i][1] = (half1[i][0] + half1[i][1]) / 2;
                    half2[i][0] = half1[i][1] + 1;
                } else{
                    half2[i][0] = half1[i][0];
                    half2[i][1] = half1[i][1];
                }
            }
        } else{
            for(unsigned i = 0; i < numParams; i++){
                if(i == halvingParam){
                    half1[i][0] = half2[i][0];
                    half1[i][1] = (half2[i][0] + half2[i][1]) / 2;
                    half2[i][0] = half1[i][1] + 1;
                } else{
                    half1[i][0] = half2[i][0];
                    half1[i][1] = half2[i][1];
                }
            }
        }

        for(unsigned i = 0; i < numParams; i++){
            if(paramRefined[i] == 0 && half1[i][0] == half2[i][1]){
                optimalParams[i] = half1[i][0];
                paramRefined[i] = 1;
                refinedParams ++;
            }
        }
    }


    strat_t optimalStrat;
    for(unsigned i = 0; i < numParams; i++){
        optimalStrat.params[i] = optimalParams[i];
    }
    optimalStrat = perfByParams(stratTypeID, strategies, optimalStrat, numStrats);

    return optimalStrat;
}