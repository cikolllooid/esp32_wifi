#ifndef BLE_HIDD_DEMO_H
#define BLE_HIDD_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/queue.h"  // ← важно! нужно до QueueHandle_t
#include <stdint.h>
#include <stdbool.h>
#include "esp_timer.h"

// Глобальные переменные
extern QueueHandle_t ble_command_queue;
extern uint8_t hidd_service_uuid128[];
extern uint16_t hid_conn_id;
extern bool sec_conn;
extern esp_timer_handle_t ble_timer;

// Функции
void stop_ble_manual(void);
void start_code(int time);
void parse_ble_line(const char *line);
void ble_line_task(void *param);

#ifdef __cplusplus
}
#endif

#endif // BLE_HIDD_DEMO_H
