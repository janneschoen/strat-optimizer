#ifndef COMMON_H
#define COMMON_H

#define MAX_PARAMS 3
#define NUM_PERF_TYPES 2
#define STRAT_STORAGE 1

typedef struct{
    float params[MAX_PARAMS]; // parameters of strategy object
    float performance[NUM_PERF_TYPES]; // ann. profit, sharpe ...
    float storage[STRAT_STORAGE]; // use case individual to strategy type
} strat_t;


void backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned start, unsigned end);

// Strategy Functions

int SMA_Crossover(unsigned day, strat_t * strategy, float * prices);

#endif