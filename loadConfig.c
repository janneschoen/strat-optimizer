#include "common.h"
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define FILENAME "execMode.json"

int loadInt(cJSON *json, const char *key) {
    cJSON *item = cJSON_GetObjectItem(json, key);
    if(!item){
        printf("ERROR: '%s' not found in execMode config.\n", key);
        exit(1);
    }
    return item ? item->valueint : 0;
}

void loadConfig(){
    FILE *file = fopen(FILENAME, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(length);
    fread(data, 1, length, file);
    fclose(file);
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(data);
        return;
    }

    config.stratTypeID = loadInt(json, "stratTypeID");
    config.btLength = loadInt(json, "btLength");
    config.fullYear = loadInt(json, "fullYear");
    config.singleTest = loadInt(json, "singleTest");
    config.goal = loadInt(json, "goal");

    cJSON *item = cJSON_GetObjectItem(json, "ticker");
    snprintf(config.ticker, sizeof(config.ticker), "%s", item->valuestring);

    cJSON * array = cJSON_GetObjectItem(json, "params");
    if (cJSON_IsArray(array)) {
        unsigned arrSize = cJSON_GetArraySize(array);
        for (unsigned i = 0; i < arrSize; i++) {
            cJSON *item = cJSON_GetArrayItem(array, i);
            config.params[i] = item->valuedouble;
        }
    }
    cJSON * array2 = cJSON_GetObjectItem(json, "gridIntv");
    if (cJSON_IsArray(array2)) {
        unsigned arrSize = cJSON_GetArraySize(array2);
        for (unsigned i = 0; i < arrSize; i++) {
            cJSON *item = cJSON_GetArrayItem(array2, i);
            config.gridIntv[i] = item->valuedouble;
        }
    }

    cJSON * array3 = cJSON_GetObjectItem(json, "visuals");
    if (cJSON_IsArray(array3)) {
        unsigned arrSize = cJSON_GetArraySize(array3);
        for (unsigned i = 0; i < arrSize; i++) {
            cJSON *item = cJSON_GetArrayItem(array3, i);
            config.visuals[i] = item->valueint;
        }
    }

    cJSON_Delete(json);
    free(data);
};