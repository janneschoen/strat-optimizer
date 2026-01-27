#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#define PRICES_MAX 600
#define MAX_PARAMS 3
#define NUM_STRAT_TYPES 1


// strat things
typedef struct{
    unsigned params[MAX_PARAMS];
    float performance;
} strat_t;

typedef struct{
    char * name;
    unsigned numParams;
    char * paramNames[MAX_PARAMS];
} stratType_t;

extern const stratType_t stratTypes[NUM_STRAT_TYPES];

unsigned getLookback(unsigned stratTypeID, strat_t * strategy);

float getPos(unsigned stratTypeID, strat_t * strategy, unsigned day, float * prices);
float genSignal0(unsigned day, strat_t * strategy, float * prices);

// struct oder array aus funktionen ??


// graphics
void loadHeading();
void clear();
void loadingBar(unsigned done, unsigned goal);


// backtesting
void getPrices(char * ticker, unsigned priceAmount, float * prices);
float backtest(unsigned stratTypeID, strat_t * strategy, float * prices, unsigned days);

// tuning
void genStrats(unsigned stratTypeID, unsigned param, strat_t * strategies, unsigned numStrats, strat_t * strategy, unsigned * stratsMade);
void testStrats(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, float * prices, unsigned priceAmount);
void findBestStrat(unsigned stratTypeID, strat_t * strategies, unsigned numStrats);

void doRegression(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, strat_t * predStrats, unsigned degree);


#endif