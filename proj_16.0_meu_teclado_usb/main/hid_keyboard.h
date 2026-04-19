#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include <stdint.h>
#include "esp_err.h"
#include "tusb.h"

// Inicializa o stack USB e o HID
esp_err_t hid_keyboard_init(void);

// Envia um array de até 6 teclas pressionadas simultaneamente
void hid_keyboard_send(uint8_t keycodes[6]);

#endif
