#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "driver/gpio.h" // Added for GPIO functions

#include "main.h"
#include "mqtt.h"

void app_main(void)
{
    wifi_init_sta();
    mqtt_app_start();
}