#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#define PRICES_FILE "temp/prices.temp"
#define STRAT_FILE "temp/strategies.temp"
#define CHART_FILE "temp/networth.temp"

#define GETPRICES_PY "getPrices.py"
#define PLOTTING_PY "plotting.py"
#define CHART_PY "chart.py"

#define MAX_PARAMS 4
#define NUM_STRAT_TYPES 2


// strat things
typedef struct{
    unsigned params[MAX_PARAMS];
    float performance;
} strat_t;

typedef float (*getSigFun)(unsigned day, strat_t * strategy, float * prices);

typedef struct{
    char * name;
    unsigned numParams;
    char * paramNames[MAX_PARAMS];
    unsigned minParams[MAX_PARAMS];
    getSigFun getSignal;
} stratType_t;

extern const stratType_t stratTypes[NUM_STRAT_TYPES];

unsigned getLookback(unsigned stratTypeID, strat_t * strategy);


// graphics
void visualise(unsigned stratTypeID, strat_t * strategies, unsigned numStrats);
void loadHeading();
void clear();
void loadingBar(unsigned done, unsigned goal);
void showSpecs(unsigned stratTypeID, strat_t * maxStrat, unsigned btLength, unsigned priceAmount, char * ticker);
void showStrat(unsigned stratTypeID, strat_t * strategy);

// backtesting
void getPrices(char * ticker, unsigned priceAmount, float * prices);
float backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned priceAmount, unsigned start, unsigned testMode);

// tuning
void genStrats(unsigned stratTypeID, unsigned param, strat_t * strategies, unsigned numStrats, strat_t * strategy, unsigned * stratsMade);
void testStrats(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, float * prices, unsigned priceAmount, unsigned start);
strat_t findBestStrat(strat_t * strategies, unsigned numStrats);

void doRegression(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, strat_t * predStrats, unsigned degree);


#endif