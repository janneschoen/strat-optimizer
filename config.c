/*
 *  config.c — CLI argument parser for the backtest engine
 *
 *  The Python orchestration layer (backtesting.py) spawns this
 *  program with key:value arguments:
 *
 *      ./compute start:300 end:2100 number_of_prices:3300 ...
 *
 *  This parser extracts each key and populates a run_config_t.
 *  It is intentionally minimal — no external libraries, no JSON.
 */

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEY_LENGTH  64
#define SEPARATOR       ':'

run_config_t load_config(int argc, char * argv[]){

    /*
     *  Nested helper: scan argv for "key:value" and copy the
     *  value portion into 'destination'.  Exits the program if
     *  the key is not found (no recovery possible).
     */
    void get_value_from_key(const char * key, char * destination){
        for(unsigned i = 1; i < argc; i++){

            char argument[MAX_KEY_LENGTH + 1 + MAX_VALUE_LENGTH];
            strcpy(argument, argv[i]);
            char read_key[MAX_KEY_LENGTH];

            for(unsigned j = 0; j < MAX_KEY_LENGTH; j++){
                if(argument[j] != SEPARATOR){
                    read_key[j] = argument[j];
                } else{
                    read_key[j] = '\0';

                    if(strcmp(read_key, key) == 0){
                        /* key matched — copy the value portion */
                        char read_value[MAX_VALUE_LENGTH];
                        for(unsigned k = 0; k < MAX_VALUE_LENGTH; k++){
                            if(argument[j + 1 + k] != '\0'){
                                read_value[k] = argument[j + 1 + k];
                            } else{
                                read_value[k] = '\0';
                                strcpy(destination, read_value);
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

    /* ---- numeric fields ------------------------------------------ */
    char buf[MAX_VALUE_LENGTH];

    get_value_from_key("number_of_prices",       buf);
    run_config.number_of_prices       = (unsigned)atoi(buf);

    get_value_from_key("number_of_combinations", buf);
    run_config.number_of_combinations = (unsigned)atoi(buf);

    get_value_from_key("number_of_parameters",   buf);
    run_config.number_of_parameters   = (unsigned)atoi(buf);

    get_value_from_key("start",                  buf);
    run_config.start                  = (unsigned)atoi(buf);

    get_value_from_key("end",                    buf);
    run_config.end                    = (unsigned)atoi(buf);

    get_value_from_key("strategy_index",         buf);
    run_config.strategy_index         = (unsigned)atoi(buf);

    get_value_from_key("trading_days",           buf);
    run_config.trading_days           = (unsigned)atoi(buf);

    /* ---- string (path) fields ------------------------------------ */
    get_value_from_key("prices_path",       run_config.prices_path);
    get_value_from_key("parameter_path",    run_config.parameter_path);
    get_value_from_key("equity_path",       run_config.equity_path);
    get_value_from_key("performances_path", run_config.performances_path);

    return run_config;
}
