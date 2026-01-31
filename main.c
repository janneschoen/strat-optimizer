#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_TICKER 5
// Arguments: stratTypeID, p0, p1, p2, btLength, ticker

int main(){

    unsigned stratTypeID;
    scanf(" %u", &stratTypeID);
    if(stratTypeID >= NUM_STRAT_TYPES){
        printf("ERROR: StratTypeID doesn't exist.\n");
        exit(1);
    }

    strat_t maxStrat;
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        scanf("%u", &maxStrat.params[i]);
        if(maxStrat.params[i] == 0){
            printf("ERROR: Param can't be 0.\n");
            exit(1);
        }
    }

    unsigned btLength;
    scanf(" %u", &btLength);

    unsigned maxLookback = getLookback(stratTypeID, &maxStrat);

    char ticker[MAX_TICKER];
    scanf(" %s", ticker);

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

    printf("%f\n", bestStrat.performance);

    free(strategies);
    return 0;
}