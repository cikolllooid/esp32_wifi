#include "esp_wifi.h"
#include "esp_wifi_types.h" // Убедитесь, что включили этот заголовок

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size) {
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false));
}

extern void* g_wifi_menuconfig;

int8_t free80211_send(uint8_t *buffer, uint16_t len) {
    int8_t rval = 0;

    // Проверьте корректность указателя и смещения
    *(uint32_t *)((uint32_t)&g_wifi_menuconfig - 0x253) = 1;

    // Отправка raw frame
    wsl_bypasser_send_raw_frame(buffer, len);

    if (rval == -4) {
        asm("ill"); // Убедитесь, что это ожидаемое поведение
    }

    return rval;
}
