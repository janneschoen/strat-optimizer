#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PROGRESS_INTV 100
#define BUDGET 10000

#define MAX_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 64
#define SEPARATOR ':'

float (*getSignal[])(unsigned day, strat_t * strategy, float * prices) = {
    SMA_crossover_signal,
    RSI_signal,
};

void backtest(unsigned stratType, strat_t * strategy, float * prices, unsigned start, unsigned end, float * equityCurve);



int main(int argc, char *argv[]){

    if(argc == 1){
        printf("Program has to be launched via main.py\n");
        exit(0);
    }

    int get_value_from_key(const char * key, char * destination){
        
        for(unsigned i = 1; i < argc; i++){

            char argument[MAX_KEY_LENGTH + 1 + MAX_VALUE_LENGTH];
            strcpy(argument, argv[i]);

            char read_key[MAX_KEY_LENGTH];

            for(unsigned j = 0; j < MAX_KEY_LENGTH; j++){

                if(argument[j] != SEPARATOR){
                    read_key[j] = argument[j];
                } else{
                    read_key[j] = '\0';
                    if(strcmp(read_key, key) == 0){ // Key match found

                        char read_value[MAX_VALUE_LENGTH];
                        for(unsigned k = 0; k < MAX_VALUE_LENGTH; k++){
                            if(argument[j+1+k] != '\0'){
                                read_value[k] = argument[j+1+k];
                            } else{
                                read_value[k] = '\0';
                                strcpy(destination, read_value);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
        printf("Error: key '%s' not found in provided data.\n", key);
        exit(1);
    }

    // Reading numeric values from given data via keys

    char str_numPrices[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_prices", str_numPrices);
    unsigned numPrices = atoi(str_numPrices);

    char str_numStrats[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_combinations", str_numStrats);
    unsigned numStrats = atoi(str_numStrats);

    char str_numParams[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_parameters", str_numParams);
    unsigned numParams = atoi(str_numParams);

    char str_lookback[MAX_VALUE_LENGTH];
    get_value_from_key("lookback", str_lookback);
    unsigned start = atoi(str_lookback);

    char str_stratType[MAX_VALUE_LENGTH];
    get_value_from_key("strategy_index", str_stratType);
    unsigned stratType = atoi(str_stratType);


    // Reading prices from file

    float prices[numPrices];

    FILE * file;
    file = fopen(argv[2], "r");
    for(unsigned i = 0; i < numPrices; i++){
        if (fscanf(file, "%f", &prices[i]) != 1) {
            printf("Error: Prices could not be properly read. ");
            printf("Possible reason: not enough prices available for chosen timeframe.\n");
            fclose(file);
            exit(1);
        }
    }
    fclose(file);

    // Reading parameter combos from file

    strat_t strategies[numStrats];

    file = fopen(argv[4], "r");

    for(unsigned i = 0; i < numStrats; i++){
        for(unsigned j = 0; j < numParams; j++){
            float param;
            if (fscanf(file, "%f", &param) != 1) {
                printf("Error: Parameter combinations could not be properly read.\n");
                fclose(file);
                exit(1);
            }
            strategies[i].params[j] = param;
        }
    }
    fclose(file);

    // Backtesting each parameter combination

    float equityCurve[numPrices-start];
    for(unsigned i = 0; i < numStrats; i++){
        if(i % PROGRESS_INTV == 0){
            printf("\r%u / %u", i, numStrats);
            fflush(stdout);
        }
        backtest(stratType, &strategies[i], prices, start, numPrices, equityCurve);
    }
    printf("\n");

    // Saving performances

    file = fopen(argv[8], "w");
    for(unsigned i = 0; i < numStrats; i++){
        fprintf(file, "%f\n", strategies[i].performance[0]);
    }
    fclose(file);

    // Saving equity curve for single test
    if(numStrats == 1){
        file = fopen(argv[9], "w");
        for(unsigned i = 0; i < numPrices-start; i++){
            fprintf(file, "%f\n", equityCurve[i]);
        }
        fclose(file);
    }

    return 0;
}

// Backtesting logic

void backtest(unsigned stratType, strat_t * strategy, float * prices, unsigned start, unsigned end, float * equityCurve){
    float cash = BUDGET, assetsOwned = 0, assetLoans = 0;
    float networth;

    for(unsigned i = start; i < end; i++){
        float knownPrices[end];
        // prices genSignal function is allowed to know at this time
        for(unsigned j = 0; j < end; j++){
            if(j <= i){
                knownPrices[j] = prices[j];
            } else{
                knownPrices[j] = NAN;
            }
        }

        networth = (assetsOwned - assetLoans) * knownPrices[i] + cash;
        equityCurve[i-start] = networth;

        if(networth <= 0){
            networth = 0;
            break;
        }

        float signal = getSignal[stratType](i, strategy, prices);

        float desInvestment = signal * networth / knownPrices[i];

        if(desInvestment > 0){ // entering long position
            cash -= (assetLoans * knownPrices[i]); // covering shorts
            assetLoans = 0;
            cash -= (desInvestment - assetsOwned) * knownPrices[i]; // buying 
            assetsOwned = desInvestment;
        } else if(desInvestment < 0){ // entering short position
            desInvestment = fabs(desInvestment);
            cash += (assetsOwned * knownPrices[i]); // closing longs
            assetsOwned = 0;
            cash += (desInvestment - assetLoans) * knownPrices[i]; // selling
            assetLoans = desInvestment;
        }
    }

    float profit = (networth - BUDGET) / BUDGET;

    strategy->performance[0] = profit;

    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy->storage[i] = NAN;
    }
}