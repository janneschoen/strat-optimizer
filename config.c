#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEY_LENGTH 64
#define SEPARATOR ':'

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