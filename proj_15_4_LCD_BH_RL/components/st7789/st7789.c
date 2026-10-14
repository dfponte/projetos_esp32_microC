#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "st7789.h"

#define PIN_NUM_BK_LIGHT 46

void st7789_init_hardware(esp_lcd_panel_io_handle_t *out_io_handle, esp_lcd_panel_handle_t *out_panel_handle, int host, const esp_lcd_panel_io_spi_config_t *io_config, const esp_lcd_panel_dev_config_t *panel_config) {
    // 1. Configurar e ligar pino do Backlight
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_BK_LIGHT),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1); 

    // 2. Criar a interface de IO do painel SPI
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)host, io_config, out_io_handle));

    // 3. Instanciar o Driver ST7789 nativo
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(*out_io_handle, panel_config, out_panel_handle));

    // 4. Resetar e inicializar fisicamente o display
    esp_lcd_panel_reset(*out_panel_handle);
    esp_lcd_panel_init(*out_panel_handle);
    
    // Sincronização padrão de cores e inversão do ST7789
    esp_lcd_panel_invert_color(*out_panel_handle, true); 
    esp_lcd_panel_disp_on_off(*out_panel_handle, true);
}
