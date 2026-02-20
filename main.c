#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

execMode_t config;

int main(){
    loadConfig();

    strat_t strategy;
    for(unsigned i = 0; i < stratTypes[config.stratTypeID].numParams; i++){
        strategy.params[i] = config.params[i];
    }

    unsigned maxLookback = getLookback(config.stratTypeID, &strategy);

    unsigned priceAmount = config.btLength + maxLookback;
    float prices[priceAmount];
    getPrices(config.ticker, priceAmount, prices);

    printf("%s | %s", config.ticker, stratTypes[config.stratTypeID].name);
    printf(" | %s | %u days", config.fullYear ? "full year" : "252 trading days", config.btLength);
    printf(" | %s \n\n", config.singleTest ? "single test" : "range test");

    if(config.singleTest){
        backtest(config.stratTypeID, &strategy, prices, maxLookback, priceAmount);
        showStrat(config.stratTypeID, &strategy);
        if(config.visuals){
            char command[50];
            sprintf(command, "python %s", CHART_PY);
            system(command);
        }
    } else{
        unsigned numStrats = 1;
        for(unsigned i = 0; i < stratTypes[config.stratTypeID].numParams; i++){
            if(stratTypes[config.stratTypeID].minParams[i] == 0){
                numStrats *= strategy.params[i] + 1;
            } else{
                numStrats *= strategy.params[i];
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

        strat_t * temp = realloc(strategies, numStrats * sizeof(strat_t));
        strategies = temp;

        printf("\n");
        for(unsigned i = 0; i < numStrats; i++){
            backtest(config.stratTypeID, &strategies[i], prices, maxLookback, priceAmount);
            if(i % 250 == 0){
                loadingBar(i, numStrats);
            }
        }
        printf("\033[F\033[2K");

        strat_t winningStrat = findBestStrat(strategies, numStrats);
        strat_t optimalStrat = findOptimalStrat(config.stratTypeID, strategies, numStrats);
        
        printf("Winner : ");
        showStrat(config.stratTypeID, &winningStrat);
        printf("Optimal: ");
        showStrat(config.stratTypeID, &optimalStrat);

        if(config.visuals){
            visualise(config.stratTypeID, strategies, numStrats);
        }
        free(strategies);
    }

 
 

    return 0;
}