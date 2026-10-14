#include "relay.h"
#include "driver/gpio.h"

void relay_init(int pin) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << pin),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(pin, 0); // Inicia desligado
}

void relay_set_status(int pin, bool status) {
    gpio_set_level(pin, status ? 1 : 0);
}