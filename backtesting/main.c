#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_ARGUMENTS 6

// Arguments: stratTypeID, params[], btLength, ticker, fullYear, visualisation

int main(int argc, char * argv[]){
    char * arguments = "stratTypeID, params[], btLength, ticker, fullYear?, visualisations?";
    if(argc == 1){
        printf("ERROR: Requires arguments: %s\n", arguments);
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

    strat_t strategy;
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        strategy.params[i] = strtoul(argv[argID], NULL, 10);
        argID ++;
    }

    unsigned btLength = strtoul(argv[argID], NULL, 10);
    argID ++;

    execMode_t config;

    char * ticker = argv[argID];
    argID ++;

    config.fullYear = strtoul(argv[argID], NULL, 10);
    argID ++;

    config.visualisation = strtoul(argv[argID], NULL, 10);
    argID ++;

    unsigned maxLookback = getLookback(stratTypeID, &strategy);

    unsigned priceAmount = btLength + maxLookback;
    float prices[priceAmount];
    getPrices(ticker, priceAmount, prices);

    printf("%s | %s", ticker, stratTypes[stratTypeID].name);
    printf(" | %s | %u days\n\n", config.fullYear ? "full year" : "252 trading days", btLength);

    backtest(stratTypeID, &strategy, prices, maxLookback, priceAmount);

    float perf = strategy.performance[0];
    float yearLen = config.fullYear ? 365 : 252;
    strategy.performance[0] = pow(1 + perf, yearLen / (priceAmount-maxLookback)) - 1;
    printf("Profit annualized\n");
    showStrat(stratTypeID, &strategy);

    return 0;
}