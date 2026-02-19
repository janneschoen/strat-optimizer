#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RISK_FREE_RATE 0.02
#define TRADING_FEE 0.001

void backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned start, unsigned end){

    float cash = BUDGET, assetsOwned = 0, assetLoans = 0;
    float networth = cash;
    float position;

    for(unsigned i = start; i < end; i++){
        networth = (assetsOwned - assetLoans) * prices[i] + cash;

        if(networth <= 0){
            networth = 0;
            break;
        }

        position = stratTypes[stratTypeID].getSignal(i, strategy, prices, networth);

        if(position > 0){
            // buying back borrowed assets (closing shorts)
            cash -= assetLoans * prices[i];
            assetLoans = 0;

            // buying or selling assets
            float desiredAssets = position / prices[i];
            cash += (assetsOwned - desiredAssets) * prices[i];
            assetsOwned = desiredAssets;

        } else if(position < 0){
            // selling all assets (closing longs)
            cash += assetsOwned * prices[i];
            assetsOwned = 0;

            // borrowing or giving back assets
            float desiredAssetLoans = -1 * position / prices[i];
            cash += (desiredAssetLoans - assetLoans) * prices[i];
            assetLoans = desiredAssetLoans;
        }
    }
    strategy->performance[0] = (networth - BUDGET) / BUDGET;
}

void getPrices(char * ticker, unsigned priceAmount, float * prices){

    char command[50];
    sprintf(command, "python %s %s %u", GETPRICES_PY, ticker, priceAmount);
    system(command);

    FILE * file;
    file = fopen(PRICES_FILE, "r");

    for(unsigned i = 0; i < priceAmount; i++){
        if (fscanf(file, "%f", &prices[i]) != 1) {
            printf("ERROR: Prices could not be properly downloaded.\n");
            fclose(file);
            exit(1);
        }
    }
    fclose(file);
}