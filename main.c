#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

execMode_t config;

int main(){
    loadConfig();

    strat_t strategy;
    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy.storage[i] = NAN;
    }
    unsigned visual = 0;
    for(unsigned i = 0; i < stratTypes[config.stratTypeID].numParams; i++){
        if(config.visuals[i]){
            visual = 1;
            break;
        }
    }
    for(unsigned i = 0; i < stratTypes[config.stratTypeID].numParams; i++){
        if(!config.singleTest){
            if(visual && config.visuals[i] == 0 && config.gridIntv[i] != 0){
                printf("ERROR: param %u must be fixed if not visualised.\n", i);
                exit(1);
            }
        }
        strategy.params[i] = config.params[i];
    }

    unsigned maxLookback = getLookback(config.stratTypeID, &strategy);

    unsigned btPrices = config.btLength[0];
    if(!config.singleTest){
        btPrices += config.btLength[1];
    }

    unsigned priceAmount = btPrices + maxLookback;
    float prices[priceAmount];
    getPrices(config.ticker, priceAmount, prices);

    printf("%s | %s", config.ticker, stratTypes[config.stratTypeID].name);
    printf(" | %s", config.fullYear ? "full year" : "252 trading days");
    printf(" | bt %ud | vld %ud", config.btLength[0], config.btLength[1]);
    printf(" | %s \n\n", config.singleTest ? "single test" : "range test");

    if(config.singleTest){
        for(unsigned i = 0; i < stratTypes[config.stratTypeID].numParams; i++){
            if(config.visuals[i]){
                config.visuals[0] = 1;
                break;
            }
        }
        printf("%ud - %ud:\n", maxLookback, priceAmount);
        backtest(config.stratTypeID, &strategy, prices, maxLookback, priceAmount);
        showStrat(config.stratTypeID, &strategy);

        if(config.visuals[0]){
            char command[50];
            sprintf(command, "python %s", CHART_PY);
            system(command);
        }
    } else{
        unsigned numStrats = 1;
        for(unsigned i = 0; i < stratTypes[config.stratTypeID].numParams; i++){
            float intv = config.gridIntv[i];
            if(stratTypes[config.stratTypeID].minParams[i] == 0){
                numStrats *= intv ?(strategy.params[i] + 1) / intv : 1;
            } else{
                numStrats *= intv ? strategy.params[i] / intv : 1;
            }
        }

        strat_t * strategies = malloc(numStrats * sizeof(strat_t));
        if(strategies == NULL){
            printf("ERROR: memory allocation failed\n");
            exit(1);
        }

        unsigned stratsMade = 0;
        genStrats(config.stratTypeID, 0, strategies, numStrats, &strategy, &stratsMade);
        numStrats = stratsMade;
        if(stratsMade == 0){
            printf("ERROR: generated 0 strategies.\n");
            exit(1);
        }

        strat_t * temp = realloc(strategies, numStrats * sizeof(strat_t));
        strategies = temp;

        printf("\n");
        for(unsigned i = 0; i < numStrats; i++){
            for(unsigned j = 0; j < STRAT_STORAGE; j++){
                strategies[i].storage[j] = NAN;
            }
            backtest(config.stratTypeID, &strategies[i], prices, maxLookback, priceAmount-config.btLength[1]);
            if(i % 250 == 0){
                loadingBar(i, numStrats);
            }
        }
        printf("\033[F\033[2K");

        unsigned numOptimal = 3;
        strat_t optimalStrats[numOptimal];
        optimalStrats[0] = findBestStrat(strategies, numStrats);
        optimalStrats[1] = findOptimalStrat(config.stratTypeID, strategies, numStrats);
        optimalStrats[2] = findOptimalStrat2(config.stratTypeID, strategies, numStrats);

        printf("Backtesting (%ud - %ud)\n", maxLookback, priceAmount-config.btLength[1]);
        for(unsigned i = 0; i < numOptimal; i++){
            backtest(config.stratTypeID, &optimalStrats[i], prices, maxLookback, priceAmount-config.btLength[1]);
        }

        for(unsigned i = 0; i < numOptimal; i++){
            printf("Strat %u:\n", i);
            showStrat(config.stratTypeID, &optimalStrats[i]);
        }

        printf("\nValidation (%ud - %ud)\n", priceAmount-config.btLength[1], priceAmount);
        for(unsigned i = 0; i < numOptimal; i++){
            backtest(config.stratTypeID, &optimalStrats[i], prices, priceAmount-config.btLength[1], priceAmount);
        }
        for(unsigned i = 0; i < numOptimal; i++){
            printf("Strat %u:\n", i);
            showStrat(config.stratTypeID, &optimalStrats[i]);
        }

        for(unsigned i = 0; i < stratTypes[config.stratTypeID].numParams; i++){
            if(config.visuals[i]){
                visualise(config.stratTypeID, strategies, numStrats);
                break;
            }
        }
        free(strategies);
    }

    return 0;
}