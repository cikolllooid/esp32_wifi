#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "webserver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "pcap_serializer.h"
#include "ble_hidd_demo.h"
#include "hid_dev.h"
#include "wifi_controller.h"
#include "attack.h"

static const char* TAG = "webserver";
#define UART_NUM UART_NUM_1  // Выбранный UART порт для ESP32
#define BUF_SIZE (1024)      // Размер буфера
#define TX_PIN 16  // Укажите TX пин ESP32
#define RX_PIN 17
ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);

static void handle_attack_command(const char *command, attack_type_t type, const char *data) {
    char *id_str = strstr(data, "id: ");
    char *time_str = strstr(data, "time: ");
    int id = 0, time = 0;

    if (id_str && time_str) {
        int parsed_id = sscanf(id_str, "id: %d", &id);
        int parsed_time = sscanf(time_str, "time: %d", &time);

        if (parsed_id == 1 && parsed_time == 1) {
            ESP_LOGI(TAG, "Parsed ID: %d, time: %d", id, time);
            attack_request_t attack_request = {
                .type = type,
                .ap_record_id = id,
                .timeout = time
            };
            ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request, sizeof(attack_request_t), portMAX_DELAY));
        } else {
            ESP_LOGE(TAG, "Error parsing values for %s command", command);
        }
    } else {
        ESP_LOGE(TAG, "Error: ID or time not found in %s command", command);
    }
}
int writed = 0;

static void showResult(uint8_t type, uint16_t content_size, char *content) {
    if (type == 0 && writed == 0) {
        ESP_LOGI(TAG, "Actual content_size: %d", content_size);
        ESP_LOG_BUFFER_HEX(TAG, &content_size, 2);
        uart_write_bytes(UART_NUM, "Hand\v", strlen("Hand\v"));
        ESP_LOGI(TAG, "HELLO");
        ESP_LOG_BUFFER_HEX(TAG, content, content_size);
        vTaskDelay(250 / portTICK_PERIOD_MS);

        uint8_t size_bytes[2] = {
            (uint8_t)(content_size & 0xFF),        // младший байт
            (uint8_t)((content_size >> 8) & 0xFF)  // старший байт
        };
        ESP_LOGI(TAG, "Size bytes to send: %02x %02x", size_bytes[0], size_bytes[1]);
        uart_write_bytes(UART_NUM, size_bytes, 2);

        vTaskDelay(250 / portTICK_PERIOD_MS);

        uart_write_bytes(UART_NUM, content, content_size);
        writed = 1;
    }
}


static void get_status() {  
    const attack_status_t statusik = *attack_get_status();

    if (statusik.state == 0) {
        writed = 0;  // сброс флага, готов к новому выводу
    }

    if ((statusik.state == 3 || statusik.state == 2) && writed == 0) {
        showResult(statusik.type, statusik.content_size, statusik.content);
    }
}

void uart_task() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uint8_t data[BUF_SIZE];

    while (1) {
        get_status();

        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS); // 20ms timeout
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Received: %s", (char *)data);

            if (ble_timer == NULL && strncmp((char *)data, "BLE ", 4) == 0) {
                int time = 0;
                char *p = strstr((char *)data, "time: ");
                if (p && sscanf(p, "time: %d", &time) == 1) {
                    ESP_LOGI(TAG, "Parsed time: %d", time);
                    start_code(time);
                } else {
                    ESP_LOGE(TAG, "Error parsing time in BLE command");
                }
                continue;
            }

            if (ble_timer && sec_conn) {
                char *msg = strdup((char *)data);
                if (msg != NULL) {
                    if (xQueueSend(ble_command_queue, &msg, 100 / portTICK_PERIOD_MS) != pdTRUE) {
                        ESP_LOGW(TAG, "BLE command queue full, dropping command");
                        free(msg);
                    }
                } else {
                    ESP_LOGE(TAG, "strdup failed");
                }
            }

            if (strncmp((char *)data, "Scan\n", len) == 0) {
                const wifictl_ap_records_t *ap_records;
                wifictl_scan_nearby_aps();
                ap_records = wifictl_get_ap_records();
                char full_resp[512] = {0}; // Increase buffer size if necessary
                size_t offset = 0;

                for (unsigned i = 0; i < ap_records->count; i++) {
                    if (i == ap_records->count - 1) {
                        offset += snprintf(
                            full_resp + offset,
                            sizeof(full_resp) - offset,
                            "%s\n\v",
                            ap_records->records[i].ssid
                        );
                    } else {
                        offset += snprintf(
                            full_resp + offset,
                            sizeof(full_resp) - offset,
                            "%s\n",
                            ap_records->records[i].ssid
                        );
                    }
                }

                uart_write_bytes(UART_NUM, full_resp, strlen(full_resp));

            } else if (strncmp((char *)data, "Deauth", 6) == 0) {
                ESP_LOGI(TAG, "Received Deauth command");
                handle_attack_command("Deauth", ATTACK_TYPE_DOS, (char *)data);
            } else if (strncmp((char *)data, "Handshake", 9) == 0) {
                ESP_LOGI(TAG, "Received Handshake command");
                handle_attack_command("Handshake", ATTACK_TYPE_HANDSHAKE, (char *)data);
            }
        }
    }
}

void webserver_run(){
    xTaskCreate(uart_task, "uart_task", 8192, NULL, 5, NULL);
}

