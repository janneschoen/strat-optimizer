#include <stdbool.h>
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#define PRICES_FILE "temp/prices.temp"
#define STRAT_FILE "temp/strategies.temp"
#define CHART_FILE "temp/networth.temp"

#define GETPRICES_PY "getPrices.py"
#define PLOTTING_PY "plotting.py"
#define CHART_PY "chart.py"

#define MAX_PARAMS 4
#define NUM_STRAT_TYPES 1
#define NUM_PERF_TYPES 2

extern const char * perfTypes[];

typedef struct{
    bool fullYear;
    bool singleTest;
    bool visualisation;
} execMode_t;

typedef struct{
    unsigned params[MAX_PARAMS];
    float performance[NUM_PERF_TYPES]; // annProfit, sharpe
} strat_t;

typedef float (*getSigFun)(unsigned day, strat_t * strategy, float * prices, float networth);
typedef bool (*validStratFun)(strat_t * strategy);

typedef struct{
    char * name;
    unsigned numParams;
    char * paramNames[MAX_PARAMS];
    unsigned minParams[MAX_PARAMS];
    getSigFun getSignal;
    validStratFun validStrat;
} stratType_t;

extern const stratType_t stratTypes[NUM_STRAT_TYPES];

unsigned getLookback(unsigned stratTypeID, strat_t * strategy);


// graphics
void visualise(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, unsigned goal);
void loadHeading();
void clear();
void loadingBar(unsigned done, unsigned goal);
void showStrat(unsigned stratTypeID, strat_t * strategy);

// backtesting
void getPrices(char * ticker, unsigned priceAmount, float * prices);
void backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount, unsigned start, execMode_t * config);

// tuning
void genStrats(unsigned stratTypeID, unsigned param, strat_t * strategies, unsigned numStrats, strat_t * strategy, unsigned * stratsMade);
void testStrats(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, float * prices, unsigned priceAmount, unsigned start, execMode_t * config);

strat_t findBestStrat(strat_t * strategies, unsigned numStrats, unsigned goal);
strat_t findOptimalStrat(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, unsigned goal);

void doRegression(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, strat_t * predStrats, unsigned degree);


#endif