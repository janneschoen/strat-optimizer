#ifndef COMMON_H
#define COMMON_H

#define MAX_PARAMS 3
#define NUM_PERF_TYPES 2
#define STRAT_STORAGE 1

typedef struct{
    float params[MAX_PARAMS]; // parameters of strategy object
    float performance[NUM_PERF_TYPES]; // ann. profit, sharpe ...
    float storage[STRAT_STORAGE]; // use case individual to strategy type
} strategy_config_t;


// Function for backtesting a single strategy config

void backtest(unsigned strategy_index, strategy_config_t * strategy, float * prices, unsigned start, unsigned end, float * equity_curve);


// Strategy Functions

float signal_SMA_crossover(unsigned day, strategy_config_t * strategy, float * prices);
float signal_RSI(unsigned day, strategy_config_t * strategy, float * prices);

#endif