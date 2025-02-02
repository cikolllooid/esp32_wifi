// main.cpp

#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "attack.h"
#include "wifi_controller.h"
#include "webserver.h"

static const char* TAG = "main";

void app_main(void) {
    ESP_LOGD(TAG, "app_main started");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifictl_mgmt_ap_start();
    attack_init();
    ESP_LOGD(TAG, "Starting webserver...");
    webserver_run();
}
