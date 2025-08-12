/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

//#include "ble_hidd_demo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_timer.h"

#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "hid_dev.h"
#include "attack.h"

#define MAX_COMMAND_LENGTH 256
#define COMMAND_QUEUE_SIZE 15

QueueHandle_t ble_command_queue = NULL;

#define HID_DEMO_TAG "HID_DEMO"

uint16_t hid_conn_id = 0;
bool sec_conn = false;
#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

#define HIDD_DEVICE_NAME            "HID"
uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

typedef struct {
    uint8_t keycode;
    uint8_t modifier;
} keymap_t;


esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = ESP_BLE_APPEARANCE_HID_KEYBOARD,       //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

keymap_t char_to_keycode(const char *ch) {
    keymap_t result = {0, 0};

    if (strlen(ch) == 1) {
        char c = ch[0];
        if (c >= 'a' && c <= 'z') {
            result.keycode = 0x04 + (c - 'a');
        } else if (c >= 'A' && c <= 'Z') {
            result.keycode = 0x04 + (c - 'A');
            result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c >= '1' && c <= '9') {
            result.keycode = 0x1E + (c - '1');
        } else if (c == '0') {
            result.keycode = 0x27;
        } else if (c == '\n') {
            result.keycode = HID_KEY_RETURN;
        } else if (c == ' ') {
            result.keycode = 0x2C;
        } else if (c == '=') {
            result.keycode = 0x2E;
        } else if (c == '/') {
            result.keycode = 0x38;
        } else if (c == '.') {
            result.keycode = 0x37;
        } else if (c == '\'') {
            result.keycode = 0x34;
        } else if (c == '!') {
            result.keycode = 0x1E; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '@') {
            result.keycode = 0x1F; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '#') {
            result.keycode = 0x20; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '$') {
            result.keycode = 0x21; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '%') {
            result.keycode = 0x22; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '^') {
            result.keycode = 0x23; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '&') {
            result.keycode = 0x24; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '*') {
            result.keycode = 0x25; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == '(') {
            result.keycode = 0x26; result.modifier = LEFT_SHIFT_KEY_MASK;
        } else if (c == ')') {
            result.keycode = 0x27; result.modifier = LEFT_SHIFT_KEY_MASK;
        }
    } else if (strcmp(ch, "esc") == 0) {
        result.keycode = HID_KEY_ESCAPE;
    }

    return result;
}


void send_key_with_modifier(uint8_t modifier, uint8_t keycode){
    esp_hidd_send_keyboard_value(hid_conn_id, modifier, &keycode, 1, 1);
    esp_hidd_send_keyboard_value(hid_conn_id, modifier, &keycode, 0, 0);
}

void send_string(const char *str) {
    char ch_str[2] = {0}; // временный буфер для одного символа

    while (*str) {
        ch_str[0] = *str++;
        keymap_t k = char_to_keycode(ch_str);
        if (k.keycode) {
            send_key_with_modifier(k.modifier, k.keycode);
            vTaskDelay(5 / portTICK_PERIOD_MS);
        }
    }
}


esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x30,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

void send_key(uint8_t keycode) {
    uint8_t keys[1] = {keycode};
    esp_hidd_send_keyboard_value(hid_conn_id, 0, keys, 1, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    keys[0] = 0;
    esp_hidd_send_keyboard_value(hid_conn_id, 0, keys, 0, 0);
}

void hold_key(const char *c) {
    keymap_t k = char_to_keycode(c); 
    esp_hidd_send_keyboard_value(hid_conn_id, k.modifier, &k.keycode, 1, 0);
}


void release_key(const char *c) {
    (void)c; 
    uint8_t keys[1] = {0};
    esp_hidd_send_keyboard_value(hid_conn_id, 0, keys, 0, 0);
}


uint8_t get_modifier_flag(const char *mod) {
    if (strcmp(mod, "CTRL") == 0) return LEFT_CONTROL_KEY_MASK;
    if (strcmp(mod, "SHIFT") == 0) return LEFT_SHIFT_KEY_MASK;
    if (strcmp(mod, "ALT") == 0) return LEFT_ALT_KEY_MASK;
    if (strcmp(mod, "GUI") == 0) return LEFT_GUI_KEY_MASK;
    return 0;
}

void parse_ble_line(const char *line_raw) {
    // Копируем строку, т.к. strtok изменяет ее
    char *line = strdup(line_raw);
    if (!line) return;

    if (strncmp(line, "DELAY ", 6) == 0) {
        int ms = atoi(line + 6);
        ESP_LOGI(HID_DEMO_TAG, "DELAY %d", ms);
        vTaskDelay(ms / portTICK_PERIOD_MS);

    } else if (strncmp(line, "STRING ", 7) == 0) {
        const char *text = line + 7;
        ESP_LOGI(HID_DEMO_TAG, "STRING: %s", text);
        send_string(text);

    } else if (strncmp(line, "ENTER", 5) == 0) {
        ESP_LOGI(HID_DEMO_TAG, "ENTER");
        send_key(HID_KEY_ENTER);

    } else if (strncmp(line, "HOLD ", 5) == 0) {
        const char *key = line + 5;
        ESP_LOGI(HID_DEMO_TAG, "HOLD %s", key);
        hold_key(key);

    } else if (strncmp(line, "RELEASE ", 8) == 0) {
        const char *key = line + 8;
        ESP_LOGI(HID_DEMO_TAG, "RELEASE %s", key);
        release_key(key);
    } else {
        char *token = strtok(line, " ");
        uint8_t modifier = 0;
        char *key = NULL;

        while (token) {
            if (strcmp(token, "CTRL") == 0 || strcmp(token, "SHIFT") == 0 || strcmp(token, "ALT") == 0 || strcmp(token, "GUI") == 0) {
                modifier |= get_modifier_flag(token);
            } else {
                key = token;
            }
            token = strtok(NULL, " ");
        }

        if (key != NULL) {
            keymap_t k = char_to_keycode(key);
            send_key_with_modifier(modifier | k.modifier, k.keycode);
        } else {
            ESP_LOGW(HID_DEMO_TAG, "Неизвестная команда: %s", line);
        }
    }

    free(line);
}


static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch(event) {
        case ESP_HIDD_EVENT_REG_FINISH: {
            if (param->init_finish.state == ESP_HIDD_INIT_OK) {
                //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
                esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
                esp_ble_gap_config_adv_data(&hidd_adv_data);

            }
            break;
        }
        case ESP_BAT_EVENT_REG: {
            break;
        }
        case ESP_HIDD_EVENT_DEINIT_FINISH:
	     break;
		case ESP_HIDD_EVENT_BLE_CONNECT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
            hid_conn_id = param->connect.conn_id;
            break;
        }
        case ESP_HIDD_EVENT_BLE_DISCONNECT: {
            sec_conn = false;
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        }
        case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
            ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->vendor_write.data, param->vendor_write.length);
            break;
        }
        case ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT");
            ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->led_write.data, param->led_write.length);
            break;
        }
        default:
            break;
    }
    return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
                ESP_LOGD(HID_DEMO_TAG, "%x:",param->ble_security.ble_req.bd_addr[i]);
            }
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            sec_conn = true;
            esp_bd_addr_t bd_addr;
            memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(HID_DEMO_TAG, "remote BD_ADDR: %08x%04x",
                    (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                    (bd_addr[4] << 8) + bd_addr[5]);
            ESP_LOGI(HID_DEMO_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
            ESP_LOGI(HID_DEMO_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
            if(!param->ble_security.auth_cmpl.success) {
                ESP_LOGE(HID_DEMO_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
            }
            break;
        default:
            break;
        }
}

void parse_ble_lines(const char *input) {
    char *copy = strdup(input);
    char *line = strtok(copy, "\n\r");

    while (line != NULL) {
        parse_ble_line(strdup(line));  // важно делать strdup(), потому что parse_ble_line освобождает строку
        vTaskDelay(100 / portTICK_PERIOD_MS);  // задержка между командами (по желанию)
        line = strtok(NULL, "\n\r");
    }

    free(copy);
}

void ble_command_processor(void *param) {
    char *command = NULL;

    while (1) {
        if (xQueueReceive(ble_command_queue, &command, portMAX_DELAY)) {
            if (strncmp(command, "LOOP ", 5) == 0) {
                int loop_count = atoi(command + 5);
                char *loop_commands[100];
                int command_count = 0;
                free(command);

                while (1) {
                    if (xQueueReceive(ble_command_queue, &command, portMAX_DELAY)) {
                        if (strncmp(command, "ENDLOOP", 7) == 0) {
                            free(command);
                            break;
                        }

                        if (command_count < 100) {
                            loop_commands[command_count++] = command;
                        } else {
                            free(command);
                        }
                    }
                }

                for (int i = 0; i < loop_count; i++) {
                    for (int j = 0; j < command_count; j++) {
                        parse_ble_lines(loop_commands[j]);
                        ESP_LOGW(HID_DEMO_TAG, "Неизвестная команда: %s", loop_commands[j]);
                        vTaskDelay(100 / portTICK_PERIOD_MS);

                    }
                }

                for (int j = 0; j < command_count; j++) {
                    free(loop_commands[j]);
                }

            } else {
                ESP_LOGW(HID_DEMO_TAG, "команда: %s", command);
                parse_ble_lines(command);
                free(command);
            }
        }
    }
}


void ble_start()
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize BLE controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return;
    }

    // Initialize HID profile
    if ((ret = esp_hidd_profile_init()) != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "%s init hidd profile failed", __func__);
    }

    // Register GAP and HID callbacks
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    // Configure security parameters
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

esp_timer_handle_t ble_timer = NULL;
static bool ble_initialized = false;

void ble_timer_callback(void* arg) {
    ESP_LOGI(HID_DEMO_TAG, "Timer expired, stopping BLE...");

    if (ble_timer != NULL) {
        esp_err_t err = esp_timer_stop(ble_timer);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(HID_DEMO_TAG, "Error stopping timer: %s", esp_err_to_name(err));
        }

        err = esp_timer_delete(ble_timer);
        if (err != ESP_OK) {
            ESP_LOGE(HID_DEMO_TAG, "Error deleting timer: %s", esp_err_to_name(err));
        }
        ble_timer = NULL; 
    }
}

void start_code(int time) {
    if (ble_timer != NULL) {
        esp_err_t err = esp_timer_stop(ble_timer);
        if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
            ESP_ERROR_CHECK(esp_timer_delete(ble_timer));
            ble_timer = NULL;
        } else {
            ESP_LOGE(HID_DEMO_TAG, "Error deleting timer: %s", esp_err_to_name(err));
        }
    }

    if (ble_command_queue == NULL) {
        ble_command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(char *));
        if (ble_command_queue == NULL) {
            ESP_LOGE(HID_DEMO_TAG, "Failed to create BLE command queue");
            return;
        }
        xTaskCreate(ble_command_processor, "ble_processor", 8192, NULL, 3, NULL);
    }

    const esp_timer_create_args_t timer_args = {
        .callback = &ble_timer_callback,
        .arg = NULL,
        .name = "ble_timer"
    };

    if (!ble_initialized) {
        ble_start();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        ble_initialized = true;
        ESP_LOGI(HID_DEMO_TAG, "BLE stack initialized.");
    } else {
        ESP_LOGW(HID_DEMO_TAG, "BLE already initialized.");
    }

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &ble_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(ble_timer, time * 1000000));
    ESP_LOGI(HID_DEMO_TAG, "BLE timer started for %d seconds", time);
}
