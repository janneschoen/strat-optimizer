#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PROGRESS_INTV 1000
#define RISK_FREE_RATE 0.02
#define TRADING_FEE 0.002
#define BUDGET 10000

float (*getSignal[])(unsigned day, strat_t * strategy, float * prices) = {
    SMA_crossover_signal,
    RSI_signal,
};

int main(int argc, char *argv[]){

    if(argc == 1){
        printf("Program has to be launched via main.py\n");
        exit(0);
    }

    // Reading prices from file

    unsigned numPrices = strtoul(argv[1], NULL, 10);
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

    unsigned numStrats = strtoul(argv[3], NULL, 10);
    strat_t strategies[numStrats];

    file = fopen(argv[4], "r");
    unsigned numParams = strtoul(argv[5], NULL, 10);

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

    unsigned stratType = strtoul(argv[6], NULL, 10);
    unsigned start = strtoul(argv[7], NULL, 10); // required lookback used as start

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

        if(desInvestment > 0){
            cash -= (assetLoans * knownPrices[i]);
            assetLoans = 0;
            cash -= (desInvestment - assetsOwned) * knownPrices[i];
            assetsOwned = desInvestment;
        } else if(desInvestment < 0){
            desInvestment = fabs(desInvestment);
            cash += (assetsOwned * knownPrices[i]);
            assetsOwned = 0;
            cash += (desInvestment - assetLoans) * knownPrices[i];
            assetLoans = desInvestment;
        }
    }

    float profit = (networth - BUDGET) / BUDGET;

    strategy->performance[0] = profit;

    for(unsigned i = 0; i < STRAT_STORAGE; i++){
        strategy->storage[i] = NAN;
    }
}