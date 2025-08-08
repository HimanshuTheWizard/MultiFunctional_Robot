#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"
#include "Wifi.h"
#include "config.h"

void mqtt_app_start(void);
void mqtt_publish(const char *topic, const char *message);
void mqtt_subscribe(const char *topic);
void process_command(const char *cmd);


#endif // MQTT_H