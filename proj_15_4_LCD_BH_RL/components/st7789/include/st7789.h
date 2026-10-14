#ifndef ST7789_H
#define ST7789_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"

void st7789_init_hardware(esp_lcd_panel_io_handle_t *out_io_handle, esp_lcd_panel_handle_t *out_panel_handle, int host, const esp_lcd_panel_io_spi_config_t *io_config, const esp_lcd_panel_dev_config_t *panel_config);

#endif // ST7789_H
