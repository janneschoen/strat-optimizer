#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#define TICKER "AAPL"
#define NUM_PRICES 1000

int main(){
    loadHeading();
    clear();

    unsigned priceAmount = NUM_PRICES;
    float prices[priceAmount];
    getPrices(TICKER, priceAmount, prices);

    strat_t maxStrat;
    unsigned stratTypeID;

    clear();
    printf("Strategy type?\n");
    for(unsigned i = 0; i < NUM_STRAT_TYPES; i++){
        printf("%u - %s\n", i, stratTypes[i].name);
    }
    do{
        scanf(" %u", &stratTypeID);
    } while(stratTypeID >= NUM_STRAT_TYPES);


    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        do{
            clear();
            printf("End of testing range?\n");
            printf("%s: ", stratTypes[stratTypeID].paramNames[i]);
            scanf("%u", &maxStrat.params[i]);
        } while(maxStrat.params[i] < 2);
    }

    clear();

    unsigned numStrats = maxStrat.params[0];
    for(unsigned i = 1; i < stratTypes[0].numParams; i++){
        numStrats *= maxStrat.params[i];
    }

    strat_t * strategies = (strat_t *)malloc(numStrats * sizeof(strat_t));

    unsigned stratsMade = 0;

    genStrats(stratTypeID, 0, strategies, numStrats, &maxStrat, &stratsMade);
    testStrats(stratTypeID, strategies, numStrats, prices, priceAmount);
    findBestStrat(stratTypeID, strategies, numStrats);

    visualise(stratTypeID, strategies, numStrats);

    free(strategies);
    return 0;
}