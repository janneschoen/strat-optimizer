#ifndef COMMON_H
#define COMMON_H

#define MAX_PARAMS 3
#define STRAT_STORAGE 1

#define MAX_VALUE_LENGTH 64

typedef struct{
    float sharpe_ratio;
    float annual_profit;
} performance_t;

typedef struct{
    float params[MAX_PARAMS]; // single configuration of parameters for strategy
    performance_t performance;
    float storage[STRAT_STORAGE]; // use case individual to strategy
} strategy_config_t;

typedef struct{
    unsigned start;
    unsigned end;
    int test_index;

    unsigned number_of_prices;
    unsigned number_of_parameters;
    unsigned number_of_combinations;

    unsigned strategy_index;
    unsigned trading_days;

    char prices_path[MAX_VALUE_LENGTH];
    char parameter_path[MAX_VALUE_LENGTH];
    char equity_path[MAX_VALUE_LENGTH];
    char performances_path[MAX_VALUE_LENGTH];
} run_config_t;


// Loading config from CLI parameters

run_config_t load_config(int argc, char * argv[]);


// Backtesting a single strategy config

void backtest(run_config_t run, strategy_config_t * strategy, float * prices, float * equity_curve);


// Strategy functions to generate trading signals

float signal_SMA_crossover(unsigned day, strategy_config_t * strategy, float * prices);
float signal_RSI(unsigned day, strategy_config_t * strategy, float * prices);

#endif