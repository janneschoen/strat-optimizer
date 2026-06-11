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
    if(!file){
        printf("Error: Could not open prices file '%s'.\n", run.prices_path);
        return 1;
    }
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

    file = fopen(run.parameter_path, "r");
    if(!file){
        printf("Error: Could not open parameters file '%s'.\n", run.parameter_path);
        return 1;
    }

    strategy_config_t combinations[run.number_of_combinations];

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

    unsigned days = run.end - run.start;

    if(run.number_of_combinations == 1){
        // Single combination: run sequentially so equity curve is accessible
        float equity_curve[days];
        backtest(run, &combinations[0], prices, equity_curve);

        file = fopen(run.equity_path, "w");
        if(!file){
            printf("Error: Could not write equity curve to '%s'.\n", run.equity_path);
            return 1;
        }
        for(unsigned i = 0; i < days; i++){
            fprintf(file, "%f\n", equity_curve[i]);
        }
        fclose(file);
        printf("\r1 / 1\n");
    } else {
        // Multiple combinations: run in parallel
        #pragma omp parallel for schedule(dynamic)
        for(unsigned i = 0; i < run.number_of_combinations; i++){
            float equity_curve[days];
            backtest(run, &combinations[i], prices, equity_curve);

            #pragma omp critical
            {
                static unsigned completed = 0;
                completed++;
                if(completed % PROGRESS_INTV == 0 || completed == run.number_of_combinations){
                    printf("\r%u / %u", completed, run.number_of_combinations);
                    fflush(stdout);
                }
            }
        }
        printf("\n");
    }

    // Saving performances

    file = fopen(run.performances_path, "w");
    if(!file){
        printf("Error: Could not write performances to '%s'.\n", run.performances_path);
        return 1;
    }
    for(unsigned i = 0; i < run.number_of_combinations; i++){
        fprintf(
            file, "%f, %f\n",
            combinations[i].performance.annual_profit,
            combinations[i].performance.sharpe_ratio
        );
    }
    fclose(file);

    return 0;
}