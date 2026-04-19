#include "hid_keyboard.h"
#include "esp_log.h"
#include "tinyusb.h"
static const uint8_t hid_report_desc[] = { TUD_HID_REPORT_DESC_KEYBOARD() };

esp_err_t hid_keyboard_init(void) {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false, 
        .configuration_descriptor = NULL,
    };
    return tinyusb_driver_install(&tusb_cfg);
}

void hid_keyboard_send(uint8_t keycodes[6]) {
    if (tud_hid_ready()) {
        tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycodes);
    }
}

// Callbacks necessários
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) { return hid_report_desc; }
uint16_t tud_hid_get_report_cb(uint8_t inst, uint8_t report_id, hid_report_type_t type, uint8_t* buffer, uint16_t reqlen) { return 0; }
void tud_hid_set_report_cb(uint8_t inst, uint8_t report_id, hid_report_type_t type, uint8_t const* buffer, uint16_t bufsize) {}
