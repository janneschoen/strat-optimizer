#include "common.h"
#include <stdio.h>
#include <stdlib.h>

void allocRows(void ** matrix, unsigned rows, unsigned cols, size_t size){
    for(unsigned i = 0; i < rows; i++){
        matrix[i] = malloc(cols * size);
    }
}

void freeMat(void ** matrix, unsigned rows){
    for(unsigned i = 0; i < rows; i++){
        free(matrix[i]);
    }
    free(matrix);
}

void doRegression(unsigned stratTypeID, strat_t * strategies, unsigned numStrats, strat_t * predStrats, unsigned degree){
    
    double * resultVec = malloc(numStrats * sizeof(double));
    for(unsigned i = 0; i < numStrats; i++){
        resultVec[i] = strategies[i].performance;
    }
    
    unsigned numParams = stratTypes[stratTypeID].numParams;
    unsigned numIntTerms = (numParams * (numParams-1)) / 2;
    unsigned numFeatures = 1 + numParams * degree + numIntTerms;

    unsigned **designMatrix = malloc(numStrats * sizeof(unsigned *));
    allocRows((void**)designMatrix, numStrats, numFeatures, sizeof(unsigned));

    unsigned rowsAdded = 0;
    for(unsigned i = 0; i < numStrats; i++){
        designMatrix[i][0] = 1;
        unsigned termsAdded = 1;

        for(unsigned j = 0; j < degree; j++){
            for(unsigned h = 0; h < numParams; h++){
                unsigned value = strategies[i].params[h];
                for(unsigned k = 0; k < j; k++){
                    value *= strategies[i].params[h];
                }
                designMatrix[i][termsAdded] = value;
                termsAdded ++;
            }
        }
        for(unsigned j = 0; j < numParams; j++){
            for(unsigned k = j+1; k < numParams; k++){
                unsigned interaction = strategies[i].params[j] * strategies[i].params[k];
                designMatrix[i][termsAdded] = interaction;
                termsAdded ++;
            }
        }
        rowsAdded ++;
        if(rowsAdded % 1000000 == 0){
            clear();
            printf("Creating design matrix...\n");
            loadingBar(rowsAdded, numStrats);
        }
    }

    unsigned ** transpose = malloc(numFeatures * sizeof(unsigned *));
    allocRows((void**)transpose, numFeatures, numStrats, sizeof(unsigned));

    for(unsigned i = 0; i < numFeatures; i++){
        for(unsigned j = 0; j < numStrats; j++){
            transpose[i][j] = designMatrix[j][i];
        }
    }

    double * crossProductVec = malloc(numFeatures * sizeof(double));
    for(unsigned i = 0; i < numFeatures; i++){
        double value = 0;
        for(unsigned j = 0; j < numStrats; j++){
            value += resultVec[j] * transpose[i][j];
        }
        crossProductVec[i] = value;
    }

    free(resultVec);

    unsigned ** covarianceMatrix = malloc(numFeatures * sizeof(unsigned *));
    allocRows((void**)covarianceMatrix, numFeatures, numFeatures, sizeof(unsigned));

    unsigned valuesAdded = 0;
    for(unsigned i = 0; i < numFeatures; i++){
        for(unsigned j = 0; j < numFeatures; j++){
            unsigned value = 0;
            for(unsigned k = 0; k < numStrats; k++){
                value += transpose[i][k] * designMatrix[k][j];
            }
            covarianceMatrix[i][j] = value;
            valuesAdded ++;
            clear();
            printf("Creating covariance matrix...\n");
            loadingBar(valuesAdded, numFeatures * numFeatures);
        }
    }

    freeMat((void**)designMatrix, numStrats);
    freeMat((void**)transpose, numFeatures);

    unsigned dim = numFeatures;

    clear();
    printf("Generating inverse...\n");
    
    double ** augMat = malloc(dim * sizeof(double *));
    allocRows((void**)augMat, dim, dim*2, sizeof(double));

    for(unsigned i = 0; i < dim; i++){
        for(unsigned j = 0; j < dim * 2; j++){
            if(j < dim){
                augMat[i][j] = covarianceMatrix[i][j];
            } else if(i == j - dim){
                augMat[i][j] = 1;
            } else{
                augMat[i][j] = 0;
            }
        }
    }

    freeMat((void**)covarianceMatrix, numFeatures);


    void normLine(unsigned i){
        double divisor = augMat[i][i];
        for(unsigned j = 0; j < dim*2; j++){
            augMat[i][j] /= divisor;
        }
    }

    void linCombSub(unsigned x, unsigned y, double z){
        for(unsigned i = 0; i < dim * 2; i++){
            augMat[x][i] -= augMat[y][i] * z;
        }
    }

    void switchLines(unsigned x, unsigned y){
        for(unsigned i = 0; i < dim*2; i++){
            double value = augMat[x][i];
            augMat[x][i] = augMat[y][i];
            augMat[y][i] = value;
        }
    }

    for(unsigned i = 0; i < dim; i++){

        unsigned timesSwitched = 0;
        while(augMat[i][i] == 0){
            if(timesSwitched == dim || i+1 == dim){
                printf("ERROR: Matrix not invertible.\n");
                exit(1);
            }

            switchLines(i, i+1);
            timesSwitched++;

        }

        normLine(i);

        for(unsigned j = 0; j < dim; j++){
            if(j != i){
                double factor = augMat[j][i];
                linCombSub(j, i, factor);

            }
        }
    }

    for(unsigned i = 0; i < dim; i++){
        for(unsigned j = 0; j < dim; j++){
            if((i == j && augMat[i][j] != 1) || (i != j && augMat[i][j] != 0)){
                printf("ERROR: invalid inversion.\n");
                exit(1);
            }
        }
    }

    double ** inverse = malloc(dim * sizeof(double *));
    allocRows((void**)inverse, dim, dim, sizeof(double));

    for(unsigned i = 0; i < dim; i++){
        for(unsigned j = 0; j < dim; j++){
            inverse[i][j] = augMat[i][j+dim];
        }
    }

    freeMat((void**)augMat, dim);

    double coefficients[dim];
    for(unsigned i = 0; i < dim; i++){
        double value = 0;
        for(unsigned j = 0; j < dim; j++){
            value += inverse[i][j] * crossProductVec[j];
        }
        coefficients[i] = value;
    }

    freeMat((void**)inverse, dim);
    free(crossProductVec);


    for(unsigned i = 0; i < numStrats; i++){
        predStrats[i] = strategies[i];
        float performance = 0;
        performance += coefficients[0];

        unsigned termsAdded = 1;
        for(unsigned h = 0; h < degree; h++){
            for(unsigned j = 0; j < numParams; j++){
                double value = predStrats[i].params[j];
                for(unsigned k = 0; k < h; k++){
                    value *= predStrats[i].params[j];
                }
                performance += coefficients[termsAdded] * value;
                termsAdded ++;
            }
        }
        for(unsigned j = 0; j < numParams; j++){
            for(unsigned k = j+1; k < numParams; k++){
                double interactionTerm = predStrats[i].params[j] * predStrats[i].params[k];
                performance += coefficients[termsAdded] * interactionTerm;
            }
        }
        predStrats[i].performance = performance;
    }

    if(0){
        for(unsigned i = 0; i < numStrats; i++){
            printf("real: %f - pred: %f\n", strategies[i].performance, predStrats[i].performance);
        }
    }
}