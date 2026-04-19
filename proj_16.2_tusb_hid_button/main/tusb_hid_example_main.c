/*
 * SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"

static const char *TAG = "USB_ARROWS";

// Definição dos Pinos para os Botões
#define BTN_UP    GPIO_NUM_4
#define BTN_DOWN  GPIO_NUM_5
#define BTN_LEFT  GPIO_NUM_6
#define BTN_RIGHT GPIO_NUM_7

/************* TinyUSB Descriptors (Mantidos do anterior) ****************/
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD))
};

const char* hid_string_descriptor[5] = {
    (char[]){0x09, 0x04}, "TinyUSB", "ESP32-S3 Teclado", "123456", "Interface HID",
};

static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) { return hid_report_descriptor; }
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}

/********* Lógica de Teclado ***************/

static void send_arrow_key(uint8_t keycode) {
    uint8_t keys[6] = {keycode, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keys);
    vTaskDelay(pdMS_TO_TICKS(20)); // Delay curto para o PC registrar
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL); // Solta a tecla
}

void app_main(void) {
    // Configuração dos 4 botões como entrada com Pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_LEFT) | (1ULL << BTN_RIGHT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Inicialização USB
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
    tusb_cfg.descriptor.string = hid_string_descriptor;
    tusb_cfg.descriptor.string_count = 5;
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    while (1) {
        if (tud_mounted()) {
            // Verifica cada botão (0 = pressionado devido ao pull-up)
            if (gpio_get_level(BTN_UP) == 0)    send_arrow_key(HID_KEY_ARROW_UP);
            if (gpio_get_level(BTN_DOWN) == 0)  send_arrow_key(HID_KEY_ARROW_DOWN);
            if (gpio_get_level(BTN_LEFT) == 0)  send_arrow_key(HID_KEY_ARROW_LEFT);
            if (gpio_get_level(BTN_RIGHT) == 0) send_arrow_key(HID_KEY_ARROW_RIGHT);
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Debounce básico
    }
}
