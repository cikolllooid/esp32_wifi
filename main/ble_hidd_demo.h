#ifndef HID_DEMO_H
#define HID_DEMO_H

#include "esp_gap_ble_api.h"
#include "esp_hidd_api.h"
#include "esp_log.h"

// Макросы и константы
#define HID_DEMO_TAG "HID_DEMO"
#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))
#define HIDD_DEVICE_NAME        "HID"

// UUID
extern uint8_t hidd_service_uuid128[];

// Параметры рекламы
extern esp_ble_adv_data_t hidd_adv_data;
extern esp_ble_adv_params_t hidd_adv_params;

// Глобальные переменные
extern uint16_t hid_conn_id;
extern bool sec_conn;

void start_code(int time);
void stop_ble_manual();

#endif // HID_DEMO_H
