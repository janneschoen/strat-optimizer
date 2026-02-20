#include "common.h"
#include <stdlib.h>


const stratType_t stratTypes[NUM_STRAT_TYPES] ={
    {"SMA crossover",
        3,
        {"SMA len", "LMA len", "Reserves"},
        {1,1,0},
        getSignal0, validStrat0},
    {"dynamic SMA crossover",
        5,
        {"Max SMA len", "Max LMA len", "Reserves max", "Training size", "Training freq"},
        {1,1,0,2,1},
        getSignal1, validStrat1}
};

const char * perfTypes[] = {
    "annualized Profit",
    "sharpe Ratio"
};

unsigned getLookback(unsigned stratTypeID, strat_t * strategy){
    unsigned lookback;
    switch(stratTypeID){
        case 0:
            lookback = strategy->params[0] > strategy->params[1] ?
            strategy->params[0] : strategy->params[1];
            return lookback;
        case 1:
            lookback = getLookback(0, strategy) + strategy->params[3];
            return lookback;
        default:
            exit(1);
    }
}