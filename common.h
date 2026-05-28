#ifndef COMMON_H
#define COMMON_H

#define MAX_PARAMS 3
#define NUM_PERF_TYPES 2
#define STRAT_STORAGE 1

#define MAX_VALUE_LENGTH 64

typedef struct{
    float params[MAX_PARAMS]; // parameters of strategy object
    float performance[NUM_PERF_TYPES]; // ann. profit, sharpe ...
    float storage[STRAT_STORAGE]; // use case individual to strategy type
} strategy_config_t;

typedef struct{
    unsigned number_of_prices;
    unsigned number_of_parameters;
    unsigned number_of_combinations;

    unsigned lookback;
    unsigned strategy_index;

    char prices_path[MAX_VALUE_LENGTH];
    char parameter_path[MAX_VALUE_LENGTH];
    char equity_path[MAX_VALUE_LENGTH];
    char performances_path[MAX_VALUE_LENGTH];
} run_config_t;


// Loading config from CLI parameters

run_config_t load_config(int argc, char * argv[]);


// Backtesting a single strategy config

void backtest(unsigned strategy_index, strategy_config_t * strategy, float * prices, unsigned start, unsigned end, float * equity_curve);


// Strategy Functions to generate trading signals

float signal_SMA_crossover(unsigned day, strategy_config_t * strategy, float * prices);
float signal_RSI(unsigned day, strategy_config_t * strategy, float * prices);

#endif