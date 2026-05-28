#include "common.h"
#include <stdio.h>

#define PROGRESS_INTV 100

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
        backtest(run, &combinations[i], prices, equity_curve);
    }
    printf("\n");

    // Saving performances

    file = fopen(run.performances_path, "w");
    for(unsigned i = 0; i < run.number_of_combinations; i++){
        fprintf(
            file, "%f, %f\n",
            combinations[i].performance.annual_profit,
            combinations[i].performance.sharpe_ratio
        );
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