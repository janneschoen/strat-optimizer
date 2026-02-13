#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_ARGUMENTS 7

// Arguments: main, testMode, visualisation, stratTypeID, p0, p1, p2, btLength, ticker, fullYear

int main(int argc, char * argv[]){
    if(argc == 1){
        printf("ERROR: Requires arguments: stratTypeID, params[], btLength, ticker\n");
        exit(1);
    }

    unsigned argID = 1;

    unsigned testMode = strtoul(argv[argID], NULL, 10);
    argID ++;

    unsigned visualisation = strtoul(argv[argID], NULL, 10);
    argID ++;

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

    unsigned fullYear = strtoul(argv[argID], NULL, 10);
    argID ++;

    unsigned maxLookback = getLookback(stratTypeID, &maxStrat);

    unsigned priceAmount = btLength + maxLookback;
    float prices[priceAmount];
    getPrices(ticker, priceAmount, prices);

    if(testMode){
        printf("%s | %s", ticker, stratTypes[stratTypeID].name);
        printf(" | %s days\n", fullYear ? "365" : "252");
        printf("Goal: ");
        if(SHARPE){
            printf("Sharpe-Ratio\n");
        } else{
            printf("Annual Profit\n");
        }
        printf("\n");
        maxStrat.performance = backtest(stratTypeID, &maxStrat, prices, priceAmount, maxLookback, 1, fullYear);
        showStrat(stratTypeID, &maxStrat, prices, priceAmount);
        if(visualisation){
            char command[50];
            sprintf(command, "python %s", CHART_PY);
            system(command);
        }
        return 0;
    }

    unsigned numStrats = 1;
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        if(stratTypes[stratTypeID].minParams[i] != 0){
            numStrats *= maxStrat.params[i];
        } else{
            numStrats *= maxStrat.params[i] + 1;
        }
    }

    strat_t * strategies = (strat_t *)malloc(numStrats * sizeof(strat_t));
    for(unsigned i = 0; i < numStrats; i++){
        strategies[i].performance = NAN;
    }

    unsigned stratsMade = 0;

    genStrats(stratTypeID, 0, strategies, numStrats, &maxStrat, &stratsMade);
    testStrats(stratTypeID, strategies, numStrats, prices, priceAmount, maxLookback, fullYear);
    
    clear();

    printf("%s - %s\n", ticker, stratTypes[stratTypeID].name);
    printf("Goal: ");
    if(SHARPE){
        printf("Sharpe-Ratio\n");
    } else{
        printf("Annual Profit\n");
    }
    printf("\n");

    strat_t bestStrat = findBestStrat(strategies, numStrats);
    printf("Best backtest:\n");
    showStrat(stratTypeID, &bestStrat, prices, priceAmount);

    printf("\n");

    strat_t optimalStrat = findOptimalStrat(stratTypeID, strategies, numStrats);
    printf("Approx. optimum:\n");
    showStrat(stratTypeID, &optimalStrat, prices, priceAmount);

    if(visualisation){
        visualise(stratTypeID, strategies, numStrats);
    }

    free(strategies);
    return 0;
}