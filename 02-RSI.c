#include "common.h"
#include <stdio.h>

// Strategy Type: Relative Strength Index

float RSI_signal(unsigned day, strat_t * strategy, float * prices){
    float pnlSums[2] = {0, 0};
    unsigned numPnl[2] = {0, 0};

    unsigned buyingThreshold = strategy->params[0];
    unsigned sellingThreshold = strategy->params[1];
    unsigned window = strategy->params[2];

    for(unsigned i = day - window; i < day; i++){
        float change = prices[i + 1] - prices[i];
        if(change > 0){
            pnlSums[0] += change;
            numPnl[0] ++;
        } else{
            pnlSums[1] -= change;
            numPnl[1] ++;
        }
    }
    float RSI;
    if(numPnl[0] > 0 && numPnl[1] > 0){
        float avgGain = (float)pnlSums[0] / numPnl[0];
        float avgLoss = (float)pnlSums[1] / numPnl[1];
        RSI = (100.0 / (1.0 + (avgGain / avgLoss)));
    } else{
        if(numPnl[0] + numPnl[1] == 0){
            RSI = 50;
        } else if(numPnl[0] == 0){
            RSI = 0;
        } else{
            RSI = 100;
        }
    }

    if(RSI < buyingThreshold){
        return 1;
    } else if(RSI > sellingThreshold){
        return -1;
    }
    return 0;
}
