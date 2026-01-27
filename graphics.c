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
}