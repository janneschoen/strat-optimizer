#include "common.h"
#include <stdio.h>

// Strategy Type: Simple Moving Average Crossover

float get_average_price(float * prices, unsigned from, unsigned to){
    float price_sum = 0;
    for(unsigned i = from; i < to; i++){
        price_sum += prices[i];
    }
    return price_sum / (to-from);
}

float signal_SMA_crossover(unsigned day, strategy_config_t * strategy_config, float * prices){

    // storage[0] stores yesterdays crossover state

    unsigned fast_average_length = strategy_config->params[0];
    unsigned slow_average_length = strategy_config->params[1];
    float fast_SMA = get_average_price(prices, day - fast_average_length, day);
    float slow_SMA = get_average_price(prices, day - slow_average_length, day);

    int signal = fast_SMA > slow_SMA ? 1 : -1; // Check crossover
    int last_signal = strategy_config->storage[0];

    float inventory_size = strategy_config->params[2];
    if(signal != last_signal){
        strategy_config->storage[0] = signal;
        return inventory_size * signal;
    }
    return 0;
}
