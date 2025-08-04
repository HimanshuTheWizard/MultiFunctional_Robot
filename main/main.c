#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/gpio.h" // Added for GPIO functions
#include "esp_err.h"
#include "freertos/semphr.h"

#define BLINK_GPIO              GPIO_NUM_2 // On most ESP32 Dev boards, GPIO2 is connected to the onboard LED
#define LEFT_IR_SENSOR          GPIO_NUM_34 // Example GPIO for left IR sensor
#define RIGHT_IR_SENSOR         GPIO_NUM_35 
#define LEFT_MOTOR_FORWARD      GPIO_NUM_32 // Example GPIO for left motor forward
#define LEFT_MOTOR_BACKWARD     GPIO_NUM_33 // Example GPIO for left motor backward
#define RIGHT_MOTOR_FORWARD     GPIO_NUM_25 // Example GPIO for right motor forward
#define RIGHT_MOTOR_BACKWARD    GPIO_NUM_26 // Example GPIO for right motor backward
#define TRIG_PIN                GPIO_NUM_27 // Example GPIO for ultrasonic trigger
#define ECHO_PIN                GPIO_NUM_14 // Example GPIO for ultrasonic echo
#define OBSTACLE_THRESHOLD_CM   20.0 // Distance threshold for obstacle avoidance in cm

// Function prototypes
void blink_task(void *pvParameter);
void Line_Following_Task(void *pvParameter);
void Obstacle_Avoidance_Task(void *pvParameter);
void Voice_Command_Task(void *pvParameter);
void Reminder_Task(void *pvParameter);
void MQTT_CMD_Task(void *pvParameter);
void Speaker_Task(void *pvParameter);
void Turn_Left(void);
void Turn_Right(void);
void Move_Forward(void);
void Move_Backward(void);
void Stop_Motors(void);
void Motor_Init(void);
void IR_Sensor_Init(void);

// Global Variables
SemaphoreHandle_t xObstacleDetectedSemaphore;

void blink_task(void *pvParameter)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void Turn_Left(void)
{
    gpio_set_level(LEFT_MOTOR_FORWARD, 0);
    gpio_set_level(LEFT_MOTOR_BACKWARD, 1);
    gpio_set_level(RIGHT_MOTOR_FORWARD, 1);
    gpio_set_level(RIGHT_MOTOR_BACKWARD, 0);

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Adjust delay as needed for turning duration
    gpio_set_level(LEFT_MOTOR_BACKWARD, 0);
    gpio_set_level(RIGHT_MOTOR_FORWARD, 0);
}

void Turn_Right(void)
{
    gpio_set_level(LEFT_MOTOR_FORWARD, 1);
    gpio_set_level(LEFT_MOTOR_BACKWARD, 0);
    gpio_set_level(RIGHT_MOTOR_FORWARD, 0);
    gpio_set_level(RIGHT_MOTOR_BACKWARD, 1);

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Adjust delay as needed for turning duration
    gpio_set_level(LEFT_MOTOR_FORWARD, 0);
    gpio_set_level(RIGHT_MOTOR_BACKWARD, 0);   
}

void Move_Forward(void)
{
    gpio_set_level(LEFT_MOTOR_FORWARD, 1);
    gpio_set_level(LEFT_MOTOR_BACKWARD, 0);
    gpio_set_level(RIGHT_MOTOR_FORWARD, 1);
    gpio_set_level(RIGHT_MOTOR_BACKWARD, 0);
}

void Move_Backward(void)
{
    gpio_set_level(LEFT_MOTOR_FORWARD, 0);
    gpio_set_level(LEFT_MOTOR_BACKWARD, 1);
    gpio_set_level(RIGHT_MOTOR_FORWARD, 0);
    gpio_set_level(RIGHT_MOTOR_BACKWARD, 1);
}

void Stop_Motors(void)
{
    gpio_set_level(LEFT_MOTOR_FORWARD, 0);
    gpio_set_level(LEFT_MOTOR_BACKWARD, 0);
    gpio_set_level(RIGHT_MOTOR_FORWARD, 0);
    gpio_set_level(RIGHT_MOTOR_BACKWARD, 0);
}

void Motor_Init(void)
{
    gpio_reset_pin(LEFT_MOTOR_FORWARD);
    gpio_reset_pin(LEFT_MOTOR_BACKWARD);
    gpio_reset_pin(RIGHT_MOTOR_FORWARD);
    gpio_reset_pin(RIGHT_MOTOR_BACKWARD);

    gpio_set_direction(LEFT_MOTOR_FORWARD, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEFT_MOTOR_BACKWARD, GPIO_MODE_OUTPUT);
    gpio_set_direction(RIGHT_MOTOR_FORWARD, GPIO_MODE_OUTPUT);
    gpio_set_direction(RIGHT_MOTOR_BACKWARD, GPIO_MODE_OUTPUT);
}

// Initialize GPIOs for IR sensors
void IR_Sensor_Init(void)
{
    gpio_reset_pin(LEFT_IR_SENSOR);
    gpio_reset_pin(RIGHT_IR_SENSOR);

    gpio_set_direction(LEFT_IR_SENSOR, GPIO_MODE_INPUT);
    gpio_set_direction(RIGHT_IR_SENSOR, GPIO_MODE_INPUT);
}

void Line_Following_Task(void *pvParameter)
{
    // Initial state: stop motors
    Stop_Motors();

    while (1) {
        int left_value = gpio_get_level(LEFT_IR_SENSOR);
        int right_value = gpio_get_level(RIGHT_IR_SENSOR);

        if (left_value && !right_value) {
            // Turn right
            Turn_Right();
        } else if (!left_value && right_value) {
            // Turn left
            Turn_Left();
        } else if (left_value && right_value) {
            // Move forward
            Move_Forward();
        } else {
            // Stop or handle error
            Stop_Motors();
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

float measure_distance_cm()
{
    // Trigger pulse
    gpio_set_level(TRIG_PIN, 0);
    ets_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    ets_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    // Wait for echo and measure pulse duration
    int64_t start_time = esp_timer_get_time();
    while (!gpio_get_level(ECHO_PIN)) {
        if (esp_timer_get_time() - start_time > 30000) return -1; // Timeout
    }

    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN)) {
        if (esp_timer_get_time() - echo_start > 30000) return -1; // Timeout
    }

    int64_t echo_end = esp_timer_get_time();
    float pulse_duration = (echo_end - echo_start);  // in microseconds

    // Speed of sound = 343 m/s = 0.0343 cm/us
    float distance_cm = (pulse_duration * 0.0343) / 2.0;

    return distance_cm;
}

void Obstacle_Avoidance_Task(void *pvParameters)
{
    Stop_Motors();
    while (1) {
        float distance = measure_distance_cm();

        if (distance > 0 && distance < OBSTACLE_THRESHOLD_CM) {
            // Obstacle detected: stop motors
            set_motor_speed(0, 0);

            // Optionally notify speaker
            xSemaphoreGive(xObstacleDetectedSemaphore);
        }

        vTaskDelay(pdMS_TO_TICKS(200));  // check every 200 ms
    }
}

void Voice_Command_Task(void *pvParameter)
{

}
void Reminder_Task(void *pvParameter)
{

}
void MQTT_CMD_Task(void *pvParameter)
{

}
void Speaker_Task(void *pvParameters)
{
    while (1) {
        if (xSemaphoreTake(xObstacleDetectedSemaphore, portMAX_DELAY)) {
            play_audio("obstacle_detected.mp3"); // Your audio function
        }
    }
}


void app_main(void)
{

    /* Print chip information */
    /*
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
    xTaskCreate(&blink_task, "blink_task", 1024, NULL, 5, NULL);
    */
    xTaskCreate(&Line_Following_Task, "LF_Task", 1024, NULL, 5, NULL);
    xTaskCreate(&Obstacle_Avoidance_Task, "OA_Task", 1024, NULL, 5, NULL);
    xTaskCreate(&Voice_Command_Task, "VC_Task", 1024, NULL, 5, NULL);
    xTaskCreate(&Reminder_Task, "Reminder_Task", 1024, NULL, 5, NULL);
    xTaskCreate(&MQTT_CMD_Task, "MQTT_Task", 1024, NULL, 5, NULL);
    xTaskCreate(&Speaker_Task, "SPKR_Task", 1024, NULL, 5, NULL);
}