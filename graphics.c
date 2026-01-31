#include "common.h"
#include <stdio.h>

#define HEADING_FILE "heading.txt"

#define HEADING_HEIGHT 7
#define HEADING_WIDTH 113

#define SCREENROWS 100
#define BAR_LENGTH 20


char heading[HEADING_HEIGHT][HEADING_WIDTH];

void loadHeading(){
    FILE * file = fopen(HEADING_FILE, "r");
    for(unsigned i = 0; i < HEADING_HEIGHT; i++){
        for(unsigned j = 0; j < HEADING_WIDTH; j++){
            heading[i][j] = fgetc(file);
        }
    }
    fclose(file);
    heading[HEADING_HEIGHT-1][HEADING_WIDTH-1] = ' ';
}

void printHeading(){
    printf("\n");
    for(unsigned i = 0; i < HEADING_HEIGHT; i++){
        for(unsigned j = 0; j < HEADING_WIDTH; j++){
            printf("%c", heading[i][j]);
        }
    }
    printf("\n");
}

void clear(){
    for(unsigned i = 0; i < SCREENROWS; i++){    
        printf("\033[F\033[2K");
    }
    printHeading();
    printf("\n\n");
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
    printf("] %.2f%%\n", progress*100);
    printf("( %u / %u )\n", done, goal);
}

void spaces(unsigned amount){
    for(unsigned i = 0; i < amount; i++){
        printf(" ");
    }
}
void line(unsigned length, unsigned thick){
    if(thick){
        for(unsigned i = 0; i < length; i++){
            printf("=");
        }
    } else{
        for(unsigned i = 0; i < length; i++){
            printf("-");
        }
        printf("\n");
    }
    printf("\n");
}
void showSpecs(unsigned stratTypeID, strat_t * maxStrat, unsigned btLength, unsigned priceAmount, char * ticker){
    line(HEADING_WIDTH/2, 0);

    printf("- Tested parameter-ranges:");
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        printf("\n");
        spaces(6);
        printf("- %s:", stratTypes[stratTypeID].paramNames[i]);
        printf(" %u ", maxStrat->params[i]);
    }
    printf("\n\n- Backtest-length: %u", btLength);
    printf("\n\n- Used prices: %u, ", priceAmount);
    printf("%s", ticker);
    printf("\n\n");
    line(HEADING_WIDTH/2, 0);
}

void showStrat(unsigned stratTypeID, strat_t * strategy){
    for(unsigned i = 0; i < stratTypes[stratTypeID].numParams; i++){
        spaces(5);
        printf("- %s: %u\n", stratTypes[stratTypeID].paramNames[i], strategy->params[i]);
    }
    printf("\n");
    line(HEADING_WIDTH/4, 1);
    printf("Result: %.2f%% / year\n", strategy->performance);
    line(HEADING_WIDTH/4, 1);
}
