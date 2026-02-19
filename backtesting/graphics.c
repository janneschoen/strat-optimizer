#include "common.h"
#include <stdio.h>

#define SCREENROWS 100
#define BAR_LENGTH 20

void clear(){
    for(unsigned i = 0; i < SCREENROWS; i++){    
        printf("\033[F\033[2K");
    }
}

void loadingBar(unsigned done, unsigned goal){
    clear();
    float progress = (float)done / goal;
    unsigned simpleProgress = (unsigned)(progress * BAR_LENGTH);
    printf("[");
    for(unsigned i = 0; i < simpleProgress; i++){
        printf("#");
    }
    for(unsigned i = 0; i < BAR_LENGTH-simpleProgress; i++){
        printf("-");
    }
    printf("] %.2f\n", progress);
}

void showStrat(unsigned stratTypeID, strat_t * strategy){
    printf("[");
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        printf(" %u ", strategy->params[i]);
    }
    float annProfit = strategy->performance[0];
    float sharpe = strategy->performance[1];
    printf("] %.2f profit | %.2f sharpe\n\n", annProfit, sharpe);
}
