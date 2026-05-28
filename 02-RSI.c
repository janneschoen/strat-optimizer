#include "common.h"
#include <stdio.h>

// Strategy Type: Relative Strength Index

float signal_RSI(unsigned day, strategy_config_t * strategy_config, float * prices){

    float gain_sum = 0;
    float loss_sum = 0;

    unsigned number_of_gains = 0;
    unsigned number_of_losses = 0;

    unsigned buy_threshold = strategy_config->params[0];
    unsigned sell_threshold = strategy_config->params[1];
    unsigned window_size = strategy_config->params[2];

    for(unsigned i = day - window_size; i < day; i++){
        float change = prices[i + 1] - prices[i];
        if(change > 0){
            gain_sum += change;
            number_of_gains ++;
        } else{
            loss_sum -= change;
            number_of_losses ++;
        }
    }
    
    float RSI;
    if(number_of_gains > 0 && number_of_losses > 0){
        float average_gain = (float)gain_sum / number_of_gains;
        float average_loss = (float)loss_sum / number_of_losses;
        RSI = (100.0 / (1.0 + (average_gain / average_loss)));
    } else{
        if(number_of_gains + number_of_losses == 0){
            RSI = 50;
        } else if(number_of_gains == 0){
            RSI = 0;
        } else{
            RSI = 100;
        }
    }

    if(RSI < buy_threshold){
        return 1;
    } else if(RSI > sell_threshold){
        return -1;
    }
    return 0;
}
