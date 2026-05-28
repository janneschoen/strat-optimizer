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

run_config_t load_config(int argc, char * argv[]){
    void get_value_from_key(const char * key, char * destination){
        for(unsigned i = 1; i < argc; i++){

            char argument[MAX_KEY_LENGTH + 1 + MAX_VALUE_LENGTH];
            strcpy(argument, argv[i]);
            char read_key[MAX_KEY_LENGTH];
            for(unsigned j = 0; j < MAX_KEY_LENGTH; j++){

                if(argument[j] != SEPARATOR){ // Read string until separator
                    read_key[j] = argument[j];
                } else{
                    read_key[j] = '\0';
                    if(strcmp(read_key, key) == 0){ // Check if key is what is looked for
                        char read_value[MAX_VALUE_LENGTH];
                        for(unsigned k = 0; k < MAX_VALUE_LENGTH; k++){
                            if(argument[j+1+k] != '\0'){ // Read string until end
                                read_value[k] = argument[j+1+k];
                            } else{
                                read_value[k] = '\0';
                                strcpy(destination, read_value); // Save value to destination
                                return;
                            }
                        }
                    }
                }
            }
        }
        printf("Error: key '%s' not found in provided data.\n", key);
        exit(1);
    }

    run_config_t run_config;

    // Read numeric values

    char str_number_of_prices[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_prices", str_number_of_prices);
    run_config.number_of_prices = atoi(str_number_of_prices);

    char str_number_of_combinations[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_combinations", str_number_of_combinations);
    run_config.number_of_combinations = atoi(str_number_of_combinations);

    char str_number_of_parameters[MAX_VALUE_LENGTH];
    get_value_from_key("number_of_parameters", str_number_of_parameters);
    run_config.number_of_parameters = atoi(str_number_of_parameters);

    char str_lookback[MAX_VALUE_LENGTH];
    get_value_from_key("lookback", str_lookback);
    run_config.lookback = atoi(str_lookback);

    char str_strategy_index[MAX_VALUE_LENGTH];
    get_value_from_key("strategy_index", str_strategy_index);
    run_config.strategy_index = atoi(str_strategy_index);

    // Read string values

    get_value_from_key("prices_path", run_config.prices_path);
    get_value_from_key("parameter_path", run_config.parameter_path);
    get_value_from_key("equity_path", run_config.equity_path);
    get_value_from_key("performances_path", run_config.performances_path);

    return run_config;
}

int main(int argc, char * argv[]){

    if(argc == 1){
        printf("Program has to be launched via main.py\n");
        return 1;
    }

    run_config_t run = load_config(argc, argv);

    // Reading prices from file

    float prices[run.number_of_prices];

    FILE * file;
    file = fopen(run.prices_path, "r");
    for(unsigned i = 0; i < run.number_of_prices; i++){
        if (fscanf(file, "%f", &prices[i]) != 1) {
            printf("Error: Prices could not be properly read. ");
            printf("Possible reason: not enough prices available for chosen timeframe.\n");
            fclose(file);
            return 1;
        }
    }
    fclose(file);

    // Reading parameter combos from file

    strategy_config_t combinations[run.number_of_combinations];

    file = fopen(run.parameter_path, "r");

    for(unsigned i = 0; i < run.number_of_combinations; i++){
        for(unsigned j = 0; j < run.number_of_parameters; j++){
            float param;
            if (fscanf(file, "%f", &param) != 1) {
                printf("Error: Parameter combinations could not be properly read.\n");
                fclose(file);
                return 1;
            }
            combinations[i].params[j] = param;
        }
    }
    fclose(file);

    // Backtesting each parameter combination

    float equity_curve[run.number_of_prices - run.lookback];
    for(unsigned i = 0; i < run.number_of_combinations; i++){
        if(i % PROGRESS_INTV == 0){
            printf("\r%u / %u", i, run.number_of_combinations);
            fflush(stdout);
        }
        backtest(run.strategy_index, &combinations[i], prices, run.lookback, run.number_of_prices, equity_curve);
    }
    printf("\n");

    // Saving performances

    file = fopen(run.performances_path, "w");
    for(unsigned i = 0; i < run.number_of_combinations; i++){
        fprintf(file, "%f\n", combinations[i].performance[0]);
    }
    fclose(file);

    // Saving equity curve for single test

    if(run.number_of_combinations == 1){
        file = fopen(run.equity_path, "w");
        for(unsigned i = 0; i < run.number_of_prices - run.lookback; i++){
            fprintf(file, "%f\n", equity_curve[i]);
        }
        fclose(file);
    }

    return 0;
}