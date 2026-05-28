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

float (*get_signal[])(unsigned day, strategy_config_t * strategy, float * prices) = {
    signal_SMA_crossover,
    signal_RSI,
};

void backtest(unsigned strategy_index, strategy_config_t * strategy, float * prices, unsigned start, unsigned end, float * equity_curve);

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

    char str_number_of_prices[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_prices", str_number_of_prices);
    unsigned number_of_prices = atoi(str_number_of_prices);

    char str_number_of_combinations[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_combinations", str_number_of_combinations);
    unsigned number_of_combinations = atoi(str_number_of_combinations);

    char str_number_of_parameters[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_parameters", str_number_of_parameters);
    unsigned number_of_parameters = atoi(str_number_of_parameters);

    char str_lookback[MAX_VALUE_LENGTH];
    get_value_from_key("lookback", str_lookback);
    unsigned start = atoi(str_lookback);

    char str_strategy_index[MAX_VALUE_LENGTH];
    get_value_from_key("strategy_index", str_strategy_index);
    unsigned strategy_index = atoi(str_strategy_index);

    char prices_path[MAX_VALUE_LENGTH];
    get_value_from_key("prices_path", prices_path);

    char parameter_path[MAX_VALUE_LENGTH];
    get_value_from_key("parameter_path", parameter_path);

    char equity_path[MAX_VALUE_LENGTH];
    get_value_from_key("equity_path", equity_path);

    char performances_path[MAX_VALUE_LENGTH];
    get_value_from_key("performances_path", performances_path);


    // Reading prices from file

    float prices[number_of_prices];

    FILE * file;
    file = fopen(prices_path, "r");
    for(unsigned i = 0; i < number_of_prices; i++){
        if (fscanf(file, "%f", &prices[i]) != 1) {
            printf("Error: Prices could not be properly read. ");
            printf("Possible reason: not enough prices available for chosen timeframe.\n");
            fclose(file);
            exit(1);
        }
    }
    fclose(file);

    // Reading parameter combos from file

    strategy_config_t combinations[number_of_combinations];

    file = fopen(parameter_path, "r");

    for(unsigned i = 0; i < number_of_combinations; i++){
        for(unsigned j = 0; j < number_of_parameters; j++){
            float param;
            if (fscanf(file, "%f", &param) != 1) {
                printf("Error: Parameter combinations could not be properly read.\n");
                fclose(file);
                exit(1);
            }
            combinations[i].params[j] = param;
        }
    }
    fclose(file);

    // Backtesting each parameter combination

    float equity_curve[number_of_prices-start];
    for(unsigned i = 0; i < number_of_combinations; i++){
        if(i % PROGRESS_INTV == 0){
            printf("\r%u / %u", i, number_of_combinations);
            fflush(stdout);
        }
        backtest(strategy_index, &combinations[i], prices, start, number_of_prices, equity_curve);
    }
    printf("\n");

    // Saving performances

    file = fopen(performances_path, "w");
    for(unsigned i = 0; i < number_of_combinations; i++){
        fprintf(file, "%f\n", combinations[i].performance[0]);
    }
    fclose(file);

    // Saving equity curve for single test
    if(number_of_combinations == 1){
        file = fopen(equity_path, "w");
        for(unsigned i = 0; i < number_of_prices - start; i++){
            fprintf(file, "%f\n", equity_curve[i]);
        }
        fclose(file);
    }

    return 0;
}

// Backtesting logic

void backtest(unsigned strategy_index, strategy_config_t * strategy_config, float * prices, unsigned start, unsigned end, float * equity_curve){
    float cash = BUDGET, assets_owned = 0, asset_loans = 0;
    float networth;

    for(unsigned i = start; i < end; i++){
        float known_prices[end]; // prices genSignal function is allowed to know at this time

        for(unsigned j = 0; j < end; j++){
            if(j <= i){
                known_prices[j] = prices[j];
            } else{
                known_prices[j] = NAN;
            }
        }

        networth = (assets_owned - asset_loans) * known_prices[i] + cash;
        equity_curve[i - start] = networth;

        if(networth <= 0){
            networth = 0;
            break;
        }

        float signal = get_signal[strategy_index](i, strategy_config, prices);

        float desired_investment = signal * networth / known_prices[i];

        if(desired_investment > 0){ // entering long position
            cash -= (asset_loans * known_prices[i]); // covering shorts
            asset_loans = 0;
            cash -= (desired_investment - assets_owned) * known_prices[i]; // buying 
            assets_owned = desired_investment;

        } else if(desired_investment < 0){ // entering short position
            desired_investment = fabs(desired_investment);
            cash += (assets_owned * known_prices[i]); // closing longs
            assets_owned = 0;
            cash += (desired_investment - asset_loans) * known_prices[i]; // selling
            asset_loans = desired_investment;
        }
    }

    float profit = (networth - BUDGET) / BUDGET;

    strategy_config->performance[0] = profit;

    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy_config->storage[i] = NAN;
    }
}