#include "mqtt.h"

static const char *TAG = "MQTT_EXAMPLE";
esp_mqtt_client_handle_t mqtt_client = NULL;


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    mqtt_client = event->client;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // Example: subscribe once connected
            //esp_mqtt_client_subscribe(mqtt_client, "/robot/command", 0);
            esp_mqtt_client_subscribe(mqtt_client, "/robot/config", 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed to topic, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Unsubscribed, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Message published, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Incoming Data:");
            printf("TOPIC: %.*s\r\n", event->topic_len, event->topic);
            printf("DATA: %.*s\r\n", event->data_len, event->data);

            if (strncmp(event->topic, "/robot/command", event->topic_len) == 0) {
                // Null-terminate the payload
                char *cmd = strndup(event->data, event->data_len);
                if (cmd) {
                    process_command(cmd);
                    free(cmd);
                }
            } else if (strncmp(event->topic, "/robot/config", event->topic_len) == 0) {
                // Null-terminate the JSON string
                char *json = strndup(event->data, event->data_len);
                if (json) {
                    parse_config_json(json);
                    free(json);
                }
            } else {
                ESP_LOGW(TAG, "Unknown topic received");
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb(event_data);
}


void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtt://broker.hivemq.com",
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}


void mqtt_publish(const char *topic, const char *message)
{
    if (mqtt_client) {
        int msg_id = esp_mqtt_client_publish(mqtt_client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Sent publish message ID: %d", msg_id);
    } else {
        ESP_LOGW(TAG, "MQTT client not started");
    }
}


void mqtt_subscribe(const char *topic)
{
    if (mqtt_client) {
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
        ESP_LOGI(TAG, "Subscribed to topic: %s, msg_id=%d", topic, msg_id);
    } else {
        ESP_LOGW(TAG, "MQTT client not started");
    }
}

void process_command(const char *cmd)
{
    ESP_LOGI(TAG, "Processing command: %s", cmd);
    // Add your command processing logic here
    // For example, you can parse the command and execute actions based on it
    if (strcmp(cmd, "start") == 0) {
        ESP_LOGI(TAG, "Starting robot...");
        // Start robot logic
    } else if (strcmp(cmd, "stop") == 0) {
        ESP_LOGI(TAG, "Stopping robot...");
        // Stop robot logic
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", cmd);
    }
}


