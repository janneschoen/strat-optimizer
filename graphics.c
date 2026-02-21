#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#define SCREENROWS 100
#define BAR_LENGTH 20


void visualise(unsigned stratTypeID, strat_t * strategies, unsigned numStrats){
    FILE * file = fopen(STRAT_FILE, "w");
    
    for(unsigned i = 0; i < numStrats; i++){
        for(unsigned j = 0; j < stratTypes[stratTypeID].numParams; j++){
            fprintf(file, "%f ", strategies[i].params[j]);
        }
        fprintf(file, "%f\n", strategies[i].performance[config.goal]);
    }

    fclose(file);
    char command[50];
    sprintf(command, "python %s --goal '%s'", PLOTTING_PY, perfTypes[config.goal]);
    system(command);
}

void clear(){
    for(unsigned i = 0; i < SCREENROWS; i++){    
        printf("\033[F\033[2K");
    }
}

void loadingBar(unsigned done, unsigned goal){
    float progress = (float)done / goal;
    unsigned simpleProgress = (unsigned)(progress * BAR_LENGTH);
    printf("\033[F\033[2K");
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
        float param = strategy->params[i];
        if(param == (unsigned)param){
            printf(" %u ", (unsigned)param);
        } else{
            printf(" %.3f ", strategy->params[i]);
        }
    }
    printf("] %.3f %s\n", strategy->performance[config.goal], perfTypes[config.goal]);
}
