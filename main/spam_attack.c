#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"

#include "spam_attack.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "string.h"
#include "esp_wifi_types.h"
#include "attack.h"
#include "esp_log.h"

#include "free80211.h"
static const char* TAG = "Spam";
static esp_timer_handle_t spam_timer;

uint8_t beacon_raw[] = {
    0x80, 0x00,                         // 0-1: Frame Control
    0x00, 0x00,                         // 2-3: Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 4-9: Destination address (broadcast)
    0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06, // 10-15: Source address
    0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06, // 16-21: BSSID
    0x00, 0x00,                         // 22-23: Sequence / fragment number
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 24-31: Timestamp
    0x64, 0x00,                         // 32-33: Beacon interval
    0x31, 0x04,                         // 34-35: Capability info
    0x00, 0x00, /* FILL CONTENT HERE */ // 36-38: SSID parameter set
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, // 39-48: Supported rates
    0x03, 0x01, 0x01,                   // 49-51: DS Parameter set
    0x05, 0x04, 0x01, 0x02, 0x00, 0x00, // 52-57: Traffic Indication Map
};

char *rick_ssids[] = {
    "ГОЙДА",
	"ГОЙДА 1",
	"ГОЙДА 2",
	"ГОЙДА 3",
	"ГОЙДА 4",
	"ГОЙДА 5",
	"ГОЙДА 6",
	"ГОЙДА 7",
	"ГОЙДА 8"
};

TaskHandle_t spamTaskHandle = NULL;

void spam_task(void *pvParameter) {
    uint8_t line = 0;
    uint16_t seqnum[TOTAL_LINES] = {0};

    for (;;) {
        vTaskDelay(100 / TOTAL_LINES / portTICK_PERIOD_MS);

        uint8_t beacon_rick[200];
        memcpy(beacon_rick, beacon_raw, BEACON_SSID_OFFSET - 1);
        beacon_rick[BEACON_SSID_OFFSET - 1] = strlen(rick_ssids[line]);
        memcpy(&beacon_rick[BEACON_SSID_OFFSET], rick_ssids[line], strlen(rick_ssids[line]));
        memcpy(&beacon_rick[BEACON_SSID_OFFSET + strlen(rick_ssids[line])],
               &beacon_raw[BEACON_SSID_OFFSET], sizeof(beacon_raw) - BEACON_SSID_OFFSET);

        beacon_rick[SRCADDR_OFFSET + 5] = line;
        beacon_rick[BSSID_OFFSET + 5] = line;

        beacon_rick[SEQNUM_OFFSET] = (seqnum[line] & 0x0f) << 4;
        beacon_rick[SEQNUM_OFFSET + 1] = (seqnum[line] & 0xff0) >> 4;
        seqnum[line]++;
        if (seqnum[line] > 0xfff) seqnum[line] = 0;

        free80211_send(beacon_rick, sizeof(beacon_raw) + strlen(rick_ssids[line]));

        if (++line >= TOTAL_LINES) line = 0;
    }
}

static esp_timer_handle_t spam_timer;

void spam_timer_callback(void* arg) {
    if (spamTaskHandle != NULL && eTaskGetState(spamTaskHandle) != eDeleted) {
        vTaskDelete(spamTaskHandle);
        spamTaskHandle = NULL;
        ESP_LOGI(TAG, "Spam task stopped!");
    }
}

void start_spam(int time) {
    xTaskCreate(spam_task, "spam_task", 2048, NULL, 4, &spamTaskHandle);
    ESP_LOGI(TAG, "Spam task started!");

    const esp_timer_create_args_t timer_args = {
        .callback = &spam_timer_callback,
        .arg = NULL,
        .name = "spam_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &spam_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(spam_timer, time * 1000000));
    ESP_LOGI(TAG, "Timer started for %d seconds", time);
}