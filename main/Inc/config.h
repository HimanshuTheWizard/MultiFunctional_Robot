#ifndef CONFIG_H
#define CONFIG_H

#include "cJSON.h"
#include "Wifi.h"

typedef struct {
    char id[32];  // node ID (e.g., "A", "Node1")
} Node;

typedef struct {
    char from[16];
    char to[16];
    float distance;
    char turn[16];  // ✅ Make it a string
} Edge;


void parse_config_json(const char *json_str);

#endif // CONFIG_H