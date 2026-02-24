#include <stdbool.h>
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#define PRICES_FILE "temp/prices.temp"
#define STRAT_FILE "temp/strategies.temp"
#define CHART_FILE "temp/networth.temp"

#define GETPRICES_PY "python/getPrices.py"
#define PLOTTING_PY "python/plotting.py"
#define CHART_PY "python/chart.py"

#define MAX_PARAMS 5
#define NUM_STRAT_TYPES 2
#define NUM_PERF_TYPES 2

#define BUDGET 1000
#define STRAT_STORAGE 3

typedef struct{
    unsigned stratTypeID;
    float params[MAX_PARAMS];
    float gridIntv[MAX_PARAMS];
    bool visuals[MAX_PARAMS];
    unsigned btLength[2];
    unsigned goal;
    char ticker[20];
    bool fullYear;
    bool singleTest;
} execMode_t;

extern execMode_t config;

typedef struct{
    unsigned storage[STRAT_STORAGE];
    float params[MAX_PARAMS];
    float performance[NUM_PERF_TYPES]; // profit, sharpe
} strat_t;

typedef float (*getSigFun)(unsigned day, strat_t * strategy, float * prices, float networth, float cash);
typedef bool (*validStratFun)(strat_t * strategy);

typedef struct{
    char * name;
    unsigned numParams;
    char * paramNames[MAX_PARAMS];
    float minParams[MAX_PARAMS];
    bool mustBeInt[MAX_PARAMS];
    getSigFun getSignal;
    validStratFun validStrat;
} stratType_t;

// strategy infos
extern const char * perfTypes[];
extern const stratType_t stratTypes[NUM_STRAT_TYPES];

unsigned getLookback(unsigned stratTypeID, strat_t * strategy);
float getSignal0(unsigned day, strat_t * strategy, float * prices, float networth, float cash);
float getSignal1(unsigned day, strat_t * strategy, float * prices, float networth, float cash);
bool validStrat0(strat_t * strategy);
bool validStrat1(strat_t * strategy);

// graphics
void visualise(unsigned stratTypeID, strat_t * strategies, unsigned numStrats);
void loadingBar(unsigned done, unsigned goal);
void showStrat(unsigned stratTypeID, strat_t * strategy);

// backtesting
void getPrices(char * ticker, unsigned priceAmount, float * prices);
void backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned start, unsigned end);

// tuning
void genStrats(unsigned stratTypeID, unsigned param, strat_t * strategies, unsigned numStrats, strat_t * strategy, unsigned * stratsMade);

strat_t findBestStrat(strat_t * strategies, unsigned numStrats);
strat_t findOptimalStrat(unsigned stratTypeID, strat_t * strategies, unsigned numStrats);
strat_t findOptimalStrat2(unsigned stratTypeID, strat_t * strategies, unsigned numStrats);

void loadConfig();

#endif