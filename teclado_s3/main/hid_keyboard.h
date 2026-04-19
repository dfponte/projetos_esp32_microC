#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include "esp_tinyusb.h"
#include "class/hid/hid_device.h"  // Agora ele vai encontrar!




esp_err_t hid_keyboard_init(void);
void hid_keyboard_send(uint8_t keycodes[6]);

#endif
