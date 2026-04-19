
#include "tinyusb.h"
#include "class/hid/hid_device.h"

// Descritor do Teclado HID
static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// Configuração do driver TinyUSB
static const tinyusb_config_t tusb_cfg = {
    .device_descriptor = NULL,
    .string_descriptor = NULL,
    .external_phy = false, 
    .configuration_descriptor = NULL,
};

esp_err_t hid_keyboard_init(void) {
    // Inicializa o driver TinyUSB
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    return ESP_OK;
}

// Implementação da função de envio
void hid_keyboard_send(uint8_t keycodes[6]) {
    if (tud_hid_ready()) {
        // O primeiro parâmetro (0) é o modificador (Shift, Ctrl, etc)
        // Para este exemplo básico, deixaremos 0.
        tud_hid_keyboard_report(0, 0, keycodes);
    }
}

// Callbacks obrigatórios do TinyUSB (mesmo que vazios)
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
}
