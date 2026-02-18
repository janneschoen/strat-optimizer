#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_ARGUMENTS 7

// Arguments: stratTypeID, p0, p1, p2, btLength, ticker, fullYear, testMode, visualisation

int main(int argc, char * argv[]){
    if(argc == 1){
        printf("ERROR: Requires arguments: stratTypeID, params[], btLength, ticker\n");
        exit(1);
    }

    unsigned argID = 1;

    unsigned stratTypeID = strtoul(argv[argID], NULL, 10);
    argID ++;

    if(stratTypeID > NUM_STRAT_TYPES - 1){
        printf("ERROR: Invalid strategytype '%u'.\n", stratTypeID);
        exit(1);
    }

    if(argc != (int)stratTypes[stratTypeID].numParams + NUM_ARGUMENTS){
        printf("ERROR: Requires %u arguments, got %u.\n", NUM_ARGUMENTS+stratTypes[stratTypeID].numParams, argc);
        exit(1);
    }

    strat_t maxStrat;
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        maxStrat.params[i] = strtoul(argv[argID], NULL, 10);
        argID ++;
    }

    unsigned btLength = strtoul(argv[argID], NULL, 10);
    argID ++;

    char * ticker = argv[argID];
    argID ++;

    execMode_t config;
    config.fullYear = strtoul(argv[argID], NULL, 10);
    argID ++;
    config.singleTest = strtoul(argv[argID], NULL, 10);
    argID ++;
    config.visualisation = strtoul(argv[argID], NULL, 10);
    argID ++;

    unsigned maxLookback = getLookback(stratTypeID, &maxStrat);

    unsigned priceAmount = btLength + maxLookback;
    float prices[priceAmount];
    getPrices(ticker, priceAmount, prices);

    if(config.singleTest){
        printf("%s | %s", ticker, stratTypes[stratTypeID].name);
        printf(" | %s\n", config.fullYear ? "full year" : "252 trading days");
        
        printf("Crosstested over %u periods\n", PERIODS);
        crossTest(stratTypeID, &maxStrat, prices, maxLookback, priceAmount, &config);
        showStrat(stratTypeID, &maxStrat);

        printf("Full backtest\n");
        backtest(stratTypeID, &maxStrat, prices, maxLookback, priceAmount, &config);
        showStrat(stratTypeID, &maxStrat);

        if(config.visualisation){
            char command[50];
            sprintf(command, "python %s", CHART_PY);
            system(command);
        }
        return 0;
    }

    unsigned numStrats = 1;
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        if(stratTypes[stratTypeID].minParams[i] == 0){
            numStrats *= maxStrat.params[i] + 1;
        } else{
            numStrats *= maxStrat.params[i] / GRID_INTERVAL;
        }
    }

    strat_t * strategies = (strat_t *)malloc(numStrats * sizeof(strat_t));
    for(unsigned i = 0; i < numStrats; i++){
        for(unsigned j = 0; j < NUM_PERF_TYPES; j++){
            strategies[i].performance[j] = NAN;
        }
    }

    unsigned stratsMade = 0;

    genStrats(stratTypeID, 0, strategies, numStrats, &maxStrat, &stratsMade);
    numStrats = stratsMade;

    testStrats(stratTypeID, strategies, numStrats, prices, maxLookback, priceAmount, &config);

    printf("%s | %s", ticker, stratTypes[stratTypeID].name);
    printf(" | %s | %u days\n\n", config.fullYear ? "full year" : "252 trading days", btLength);

    printf("Backtest winners\n");
    strat_t bestStrats[NUM_PERF_TYPES];
    for(unsigned i = 0; i < NUM_PERF_TYPES; i++){
        bestStrats[i] = findBestStrat(strategies, numStrats, i);
        printf("%s:\n", perfTypes[i]);
        showStrat(stratTypeID, &bestStrats[i]);
    }

    printf("Optimal\n");
    strat_t optimalStrats[NUM_PERF_TYPES];
    for(unsigned i = 0; i < NUM_PERF_TYPES; i++){
        optimalStrats[i] = findOptimalStrat(stratTypeID, strategies, numStrats, i);
        printf("%s:\n", perfTypes[i]);
        showStrat(stratTypeID, &optimalStrats[i]);
    }


    if(config.visualisation){
        for(unsigned i = 0; i < NUM_PERF_TYPES; i++){
            visualise(stratTypeID, strategies, numStrats, i);
        }
    }

    free(strategies);
    return 0;
}