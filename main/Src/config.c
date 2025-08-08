#include "config.h"

#define MAX_NODES 50
#define MAX_EDGES 100

Node nodes[MAX_NODES];
Edge edges[MAX_EDGES];
int node_count = 0;
int edge_count = 0;

static const char *TAG = "JSON_CONFIG";

void parse_config_json(const char *json_str)
{
    ESP_LOGI(TAG, "Parsing robot config JSON");

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }

    // --- Parse Nodes ---
    cJSON *node_array = cJSON_GetObjectItem(root, "nodes");
    if (cJSON_IsArray(node_array)) {
        node_count = 0;
        cJSON *node_item;
        cJSON_ArrayForEach(node_item, node_array) {
            if (node_count >= MAX_NODES) break;

            if (cJSON_IsObject(node_item)) {
                cJSON *id = cJSON_GetObjectItem(node_item, "id");
                if (cJSON_IsString(id)) {
                    strncpy(nodes[node_count].id, id->valuestring, sizeof(nodes[node_count].id) - 1);
                    nodes[node_count].id[sizeof(nodes[node_count].id) - 1] = '\0';
                    ESP_LOGI(TAG, "Node[%d]: %s", node_count, nodes[node_count].id);
                    node_count++;
                }
            }
        }
        ESP_LOGI(TAG, "Total nodes parsed: %d", node_count);
    } else {
        ESP_LOGW(TAG, "No valid 'nodes' array in config");
    }

    // --- Parse Edges ---
    cJSON *edge_array = cJSON_GetObjectItem(root, "edges");
    if (cJSON_IsArray(edge_array)) {
        edge_count = 0;
        cJSON *edge_item;
        cJSON_ArrayForEach(edge_item, edge_array) {
            if (edge_count >= MAX_EDGES) break;

            if (cJSON_IsObject(edge_item)) {
                cJSON *from = cJSON_GetObjectItem(edge_item, "from");
                cJSON *to = cJSON_GetObjectItem(edge_item, "to");
                cJSON *distance = cJSON_GetObjectItem(edge_item, "distance");
                cJSON *turn = cJSON_GetObjectItem(edge_item, "turn");

                if (cJSON_IsString(from) && cJSON_IsString(to) &&
                    cJSON_IsNumber(distance) && cJSON_IsString(turn)) {

                    strncpy(edges[edge_count].from, from->valuestring, sizeof(edges[edge_count].from) - 1);
                    strncpy(edges[edge_count].to, to->valuestring, sizeof(edges[edge_count].to) - 1);
                    strncpy(edges[edge_count].turn, turn->valuestring, sizeof(edges[edge_count].turn) - 1);

                    edges[edge_count].from[sizeof(edges[edge_count].from) - 1] = '\0';
                    edges[edge_count].to[sizeof(edges[edge_count].to) - 1] = '\0';
                    edges[edge_count].turn[sizeof(edges[edge_count].turn) - 1] = '\0';

                    edges[edge_count].distance = distance->valuedouble;

                    ESP_LOGI(TAG, "Edge[%d]: %s -> %s | Distance: %.2f | Turn: %s",
                             edge_count,
                             edges[edge_count].from,
                             edges[edge_count].to,
                             edges[edge_count].distance,
                             edges[edge_count].turn);
                    edge_count++;
                }
            }
        }
        ESP_LOGI(TAG, "Total edges parsed: %d", edge_count);
    } else {
        ESP_LOGW(TAG, "No valid 'edges' array in config");
    }

    cJSON_Delete(root);
}

