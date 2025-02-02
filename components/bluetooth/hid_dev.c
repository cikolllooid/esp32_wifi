/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "hid_dev.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_log.h"

static hid_report_map_t *hid_dev_rpt_tbl;
static uint8_t hid_dev_rpt_tbl_Len;

static hid_report_map_t *hid_dev_rpt_by_id(uint8_t id, uint8_t type)
{
    hid_report_map_t *rpt = hid_dev_rpt_tbl;

    for (uint8_t i = hid_dev_rpt_tbl_Len; i > 0; i--, rpt++) {
        if (rpt->id == id && rpt->type == type && rpt->mode == hidProtocolMode) {
            return rpt;
        }
    }

    return NULL;
}

void hid_dev_register_reports(uint8_t num_reports, hid_report_map_t *p_report)
{
    hid_dev_rpt_tbl = p_report;
    hid_dev_rpt_tbl_Len = num_reports;
    return;
}

void hid_dev_send_report(esp_gatt_if_t gatts_if, uint16_t conn_id,
                                    uint8_t id, uint8_t type, uint8_t length, uint8_t *data)
{
    hid_report_map_t *p_rpt;

    // get att handle for report
    if ((p_rpt = hid_dev_rpt_by_id(id, type)) != NULL) {
        // if notifications are enabled
        ESP_LOGD(HID_LE_PRF_TAG, "%s(), send the report, handle = %d", __func__, p_rpt->handle);
        esp_ble_gatts_send_indicate(gatts_if, conn_id, p_rpt->handle, length, data, false);
    }

    return;
}

void hid_consumer_build_report(uint8_t *buffer, consumer_cmd_t cmd)
{
    if (!buffer) {
        ESP_LOGE(HID_LE_PRF_TAG, "%s(), the buffer is NULL, hid build report failed.", __func__);
        return;
    }

    switch (cmd) {
        case HID_CONSUMER_CHANNEL_UP:
            HID_CC_RPT_SET_CHANNEL(buffer, HID_CC_RPT_CHANNEL_UP);
            break;

        case HID_CONSUMER_CHANNEL_DOWN:
            HID_CC_RPT_SET_CHANNEL(buffer, HID_CC_RPT_CHANNEL_DOWN);
            break;

        case HID_CONSUMER_VOLUME_UP:
            HID_CC_RPT_SET_VOLUME_UP(buffer);
            break;

        case HID_CONSUMER_VOLUME_DOWN:
            HID_CC_RPT_SET_VOLUME_DOWN(buffer);
            break;

        case HID_CONSUMER_MUTE:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_MUTE);
            break;

        case HID_CONSUMER_POWER:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_POWER);
            break;

        case HID_CONSUMER_RECALL_LAST:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_LAST);
            break;

        case HID_CONSUMER_ASSIGN_SEL:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_ASSIGN_SEL);
            break;

        case HID_CONSUMER_PLAY:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_PLAY);
            break;

        case HID_CONSUMER_PAUSE:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_PAUSE);
            break;

        case HID_CONSUMER_RECORD:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_RECORD);
            break;

        case HID_CONSUMER_FAST_FORWARD:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_FAST_FWD);
            break;

        case HID_CONSUMER_REWIND:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_REWIND);
            break;

        case HID_CONSUMER_SCAN_NEXT_TRK:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_SCAN_NEXT_TRK);
            break;

        case HID_CONSUMER_SCAN_PREV_TRK:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_SCAN_PREV_TRK);
            break;

        case HID_KEY_A:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_A);
            break;

        case HID_KEY_B:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_B);
            break;

        case HID_KEY_C:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_C);
            break;

        case HID_KEY_D:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_D);
            break;

        case HID_KEY_E:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_E);
            break;

        case HID_KEY_F:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F);
            break;

        case HID_KEY_G:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_G);
            break;

        case HID_KEY_H:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_H);
            break;

        case HID_KEY_I:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_I);
            break;

        case HID_KEY_J:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_J);
            break;

        case HID_KEY_K:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_K);
            break;

        case HID_KEY_L:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_L);
            break;

        case HID_KEY_M:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_M);
            break;

        case HID_KEY_N:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_N);
            break;

        case HID_KEY_O:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_O);
            break;

        case HID_KEY_P:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_P);
            break;

        case HID_KEY_Q:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_Q);
            break;

        case HID_KEY_R:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_R);
            break;

        case HID_KEY_S:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_S);
            break;

        case HID_KEY_T:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_T);
            break;

        case HID_KEY_U:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_U);
            break;

        case HID_KEY_V:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_V);
            break;

        case HID_KEY_W:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_W);
            break;

        case HID_KEY_X:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_X);
            break;

        case HID_KEY_Y:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_Y);
            break;

        case HID_KEY_Z:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_Z);
            break;

        case HID_KEY_1:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_1);
            break;

        case HID_KEY_2:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_2);
            break;

        case HID_KEY_3:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_3);
            break;

        case HID_KEY_4:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_4);
            break;

        case HID_KEY_5:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_5);
            break;

        case HID_KEY_6:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_6);
            break;

        case HID_KEY_7:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_7);
            break;

        case HID_KEY_8:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_8);
            break;

        case HID_KEY_9:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_9);
            break;

        case HID_KEY_0:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_0);
            break;

        case HID_KEY_RETURN:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_RETURN);
            break;

        case HID_KEY_ESCAPE:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_ESCAPE);
            break;

        case HID_KEY_DELETE:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_DELETE);
            break;

        case HID_KEY_TAB:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_TAB);
            break;

        case HID_KEY_SPACEBAR:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_SPACEBAR);
            break;

        case HID_KEY_MINUS:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_MINUS);
            break;

        case HID_KEY_EQUAL:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_EQUAL);
            break;

        case HID_KEY_LEFT_BRKT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_LEFT_BRKT);
            break;

        case HID_KEY_BACK_SLASH:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_BACK_SLASH);
            break;

        case HID_KEY_SEMI_COLON:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_SEMI_COLON);
            break;

        case HID_KEY_SGL_QUOTE:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_SGL_QUOTE);
            break;

        case HID_KEY_GRV_ACCENT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_GRV_ACCENT);
            break;

        case HID_KEY_COMMA:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_COMMA);
            break;

        case HID_KEY_DOT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_DOT);
            break;

        case HID_KEY_FWD_SLASH:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_FWD_SLASH);
            break;
        
        case HID_KEY_CAPS_LOCK:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_CAPS_LOCK);
            break;

        case HID_KEY_F1:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F1);
            break;

        case HID_KEY_F2:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F2);
            break;

        case HID_KEY_F3:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F3);
            break;

        case HID_KEY_F4:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F4);
            break;

        case HID_KEY_F5:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F5);
            break;

        case HID_KEY_F6:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F6);
            break;

        case HID_KEY_F7:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F7);
            break;

        case HID_KEY_F8:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F8);
            break;

        case HID_KEY_F9:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F9);
            break;

        case HID_KEY_F10:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F10);
            break;

        case HID_KEY_F11:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F11);
            break;

        case HID_KEY_F12:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_F12);
            break;

        case HID_KEY_PRNT_SCREEN:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_PRNT_SCREEN);
            break;

        case HID_KEY_SCROLL_LOCK:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_SCROLL_LOCK);
            break;

        case HID_KEY_PAUSE:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_PAUSE);
            break;

        case HID_KEY_INSERT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_INSERT);
            break;

        case HID_KEY_HOME:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_HOME);
            break;

        case HID_KEY_PAGE_UP:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_PAGE_UP);
            break;

        case HID_KEY_DELETE_FWD:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_DELETE_FWD);
            break;

        case HID_KEY_END:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_END);
            break;

        case HID_KEY_PAGE_DOWN:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_PAGE_DOWN);
            break;

        case HID_KEY_RIGHT_ARROW:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_RIGHT_ARROW);
            break;

        case HID_KEY_LEFT_ARROW:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_LEFT_ARROW);
            break;

        case HID_KEY_DOWN_ARROW:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_DOWN_ARROW);
            break;

        case HID_KEY_UP_ARROW:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_UP_ARROW);
            break;

        case HID_KEY_NUM_LOCK:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_NUM_LOCK);
            break;

        case HID_KEY_DIVIDE:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_DIVIDE);
            break;

        case HID_KEY_MULTIPLY:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_MULTIPLY);
            break;

        case HID_KEY_SUBTRACT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_SUBTRACT);
            break;

        case HID_KEY_ADD:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_ADD);
            break;

        case HID_KEY_ENTER:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_ENTER);
            break;

        case HID_KEYPAD_1:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_1);
            break;

        case HID_KEYPAD_2:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_2);
            break;

        case HID_KEYPAD_3:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_3);
            break;

        case HID_KEYPAD_4:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_4);
            break;

        case HID_KEYPAD_5:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_5);
            break;

        case HID_KEYPAD_6:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_6);
            break;

        case HID_KEYPAD_7:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_7);
            break;

        case HID_KEYPAD_8:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_8);
            break;

        case HID_KEYPAD_9:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_9);
            break;

        case HID_KEYPAD_0:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_0);
            break;

        case HID_KEYPAD_DOT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEYPAD_DOT);
            break;

        case HID_KEY_MUTE:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_MUTE);
            break;

        case HID_KEY_VOLUME_UP:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_VOLUME_UP);
            break;

        case HID_KEY_LEFT_CTRL:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_LEFT_CTRL);
            break;

        case HID_KEY_LEFT_SHIFT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_LEFT_SHIFT);
            break;

        case HID_KEY_LEFT_GUI:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_LEFT_GUI);
            break;

        case HID_KEY_RIGHT_CTRL:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_RIGHT_CTRL);
            break;

        case HID_KEY_RIGHT_SHIFT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_RIGHT_SHIFT);
            break;

        case HID_KEY_RIGHT_ALT:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_RIGHT_ALT);
            break;

        case HID_KEY_RIGHT_GUI:
            HID_CC_RPT_SET_KEY(buffer, HID_KEY_RIGHT_GUI);
            break;

        default:
            break;
    }

    return;
}