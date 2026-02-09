#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_ARGUMENTS 5

// Arguments: main, testMode, stratTypeID, p0, p1, p2, btLength, ticker

int main(int argc, char * argv[]){
    if(argc == 1){
        printf("ERROR: Requires arguments: stratTypeID, params[], btLength, ticker\n");
        exit(1);
    }

    unsigned argID = 1;

    unsigned testMode = strtoul(argv[argID], NULL, 10);
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

    unsigned maxLookback = getLookback(stratTypeID, &maxStrat);

    unsigned priceAmount = btLength + maxLookback;
    float prices[priceAmount];
    getPrices(ticker, priceAmount, prices);

    if(testMode){
        maxStrat.performance = backtest(stratTypeID, &maxStrat, prices, priceAmount, maxLookback, 1);
        showStrat(stratTypeID, &maxStrat);
        char command[50];
        sprintf(command, "python %s", CHART_PY);
        system(command);
        return 0;
    }

    unsigned numStrats = maxStrat.params[0];
    for(unsigned i = 1; i < stratTypes[0].numParams; i++){
        numStrats *= maxStrat.params[i];
    }

    strat_t * strategies = (strat_t *)malloc(numStrats * sizeof(strat_t));

    unsigned stratsMade = 0;

    genStrats(stratTypeID, 0, strategies, numStrats, &maxStrat, &stratsMade);
    testStrats(stratTypeID, strategies, numStrats, prices, priceAmount, maxLookback);

    strat_t bestStrat = findBestStrat(strategies, numStrats);

    clear();

    printf("Best backtest:\n");
    showStrat(stratTypeID, &bestStrat);


    unsigned lowerHalf[2] = {0, numStrats/2};
    unsigned upperHalf[2] = {lowerHalf[1], numStrats};
    
    while(lowerHalf[0] != lowerHalf[1] || upperHalf[0] == upperHalf[1]){
        float lowerSum = 0;
        float upperSum = 0;
        for(unsigned i = lowerHalf[0]; i < lowerHalf[1]; i++){
            lowerSum += strategies[i].performance;
        }
        for(unsigned i = upperHalf[0]; i < upperHalf[1]; i++){
            upperSum += strategies[i].performance;
        }

        if(upperSum > lowerSum){
            lowerHalf[0] = upperHalf[0];
            lowerHalf[1] = (upperHalf[1] + upperHalf[0]) / 2;
            upperHalf[0] = lowerHalf[1];
        } else{
            upperHalf[1] = lowerHalf[1];
            upperHalf[0] = (lowerHalf[1] + lowerHalf[0]) / 2;
            lowerHalf[1] = upperHalf[0];
        }
    }
    strat_t optimalStrat;

    if(strategies[upperHalf[0]].performance > strategies[upperHalf[1]].performance){
        optimalStrat = strategies[upperHalf[0]];
        //printf("id: %u\n", upperHalf[0]);
    } else{
        optimalStrat = strategies[upperHalf[1]];
        //printf("id: %u\n", upperHalf[1]);
    }

    printf("Optimal Strat (via recursive halving):\n");
    showStrat(stratTypeID, &optimalStrat);

    visualise(stratTypeID, strategies, numStrats);

    free(strategies);
    return 0;
}