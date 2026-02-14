#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


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

void testStrats(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, float * prices, unsigned priceAmount, unsigned start, execMode_t * config){
    for(unsigned i = 0; i < numStrats; i++){
        strategies[i].performance = backtest(stratTypeID, &strategies[i], prices, priceAmount, start, config);
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

float perfByParams(unsigned stratTypeID, strat_t * strategies, strat_t strategy, unsigned numStrats){
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
    return strategies[index].performance;
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

    strat_t optimalStrat;
    for(unsigned i = 0; i < numParams; i++){
        optimalStrat.params[i] = optimalParams[i];
    }
    optimalStrat.performance = perfByParams(stratTypeID, strategies, optimalStrat, numStrats);

    return optimalStrat;
}

strat_t findOptimalStrat2(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
    unsigned numParams = stratTypes[stratTypeID].numParams;
    unsigned bestParams[numParams];

    for(unsigned i = 0; i < numParams; i++){

        unsigned bestParam = stratTypes[stratTypeID].minParams[i];
        float bestPerfSum = 0;
        for(unsigned k = 0; k < numStrats; k++){
            if(strategies[k].params[i] == stratTypes[stratTypeID].minParams[i]){
                bestPerfSum += strategies[k].performance;
            }
        }

        for(unsigned j = 1; j < strategies[numStrats-1].params[i]; j++){
            float perfSum = 0;
            for(unsigned k = 0; k < numStrats; k++){
                if(strategies[k].params[i] == j){
                    perfSum += strategies[k].performance;
                }
            }
            if(perfSum > bestPerfSum){
                bestPerfSum = perfSum;
                bestParam = j;
            }
        }
        bestParams[i] = bestParam;
    }

    strat_t optimalStrat;
    for(unsigned i = 0; i < numParams; i++){
        optimalStrat.params[i] = bestParams[i];
    }
    optimalStrat.performance = perfByParams(stratTypeID, strategies, optimalStrat, numStrats);

    return optimalStrat;
}

strat_t findOptimalStrat3(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
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
                        value1 += strategies[i].performance;
                    }
                }
                if(param >= half2[j][0] && param <= half2[j][1]){
                    matching2 ++;
                    if(matching2 == numParams){
                        value2 += strategies[i].performance;
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
    optimalStrat.performance = perfByParams(stratTypeID, strategies, optimalStrat, numStrats);

    return optimalStrat;
}