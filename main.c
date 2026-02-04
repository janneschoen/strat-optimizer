#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_ARGUMENTS 4
#define RESULTS_FILE "results.out"

// Arguments: main, stratTypeID, p0, p1, p2, btLength, ticker

int main(int argc, char * argv[]){

    unsigned argID = 1;

    unsigned stratTypeID = strtoul(argv[argID], NULL, 10);

    if(argc != (int)stratTypes[stratTypeID].numParams + NUM_ARGUMENTS){
        printf("ERROR: Requires %u arguments, got %u.\n", NUM_ARGUMENTS+stratTypes[stratTypeID].numParams, argc);
        exit(1);
    }

    if(stratTypeID > NUM_STRAT_TYPES - 1){
        printf("ERROR: Invalid strategytype '%u'.\n", stratTypeID);
        exit(1);
    }
    argID ++;

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
    for(unsigned i = 0; i < 3; i++){
        printf(" %u ", bestStrat.params[i]);
    }
    printf("| %.2f \n", bestStrat.performance);

    strat_t weightedStrat;
    weightedStrat.performance = 0;
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        weightedStrat.params[i] = 0;
    }
    float perfSum = 0;
    for(unsigned i = 0; i < numStrats; i++){
        perfSum += strategies[i].performance;
    }
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        float compiledParam = 0;
        for(unsigned j = 0; j < numStrats; j++){
            compiledParam += strategies[j].params[i] * strategies[j].performance;
        }
        weightedStrat.params[i] = (unsigned)(compiledParam / perfSum);
    }
    unsigned matchFound = 0;
    for(unsigned i = 0; i < numStrats && !matchFound; i++){
        for(unsigned j = 0; j < stratTypes[stratTypeID].numParams; j++){
            if(weightedStrat.params[j] != strategies[i].params[j]){
                break;
            } else if(j == stratTypes[stratTypeID].numParams - 1){
                weightedStrat.performance = strategies[i].performance;
                matchFound = 1;
            }
        }
    }
    printf("Weighted Strategy:\n");
    for(unsigned i = 0; i < 3; i++){
        printf(" %u ", weightedStrat.params[i]);
    }
    printf("| (%.2f) \n", weightedStrat.performance);

    /*FILE * file = fopen(RESULTS_FILE, "a");
    fprintf(file, "%u ", stratTypeID);
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        fprintf(file, "%u ", bestStrat.params[i]);
    }
    fprintf(file, "%u ", btLength);
    fprintf(file, "%s ", ticker);
    fprintf(file, "%f", bestStrat.performance);
    fprintf(file, "\n");
    fclose(file);*/

    free(strategies);
    return 0;
}