#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "st7789.h"

static const char *TAG = "st7789_component";

// --- CONFIGURAÇÃO DE PINOS ESP32-S3 (DISPLAY) ---
#define LCD_HOST         SPI2_HOST
#define PIN_NUM_SCLK     12
#define PIN_NUM_MOSI     11
#define PIN_NUM_LCD_DC   9
#define PIN_NUM_LCD_CS   10  
#define PIN_NUM_LCD_RST  3   
#define PIN_NUM_BK_LIGHT 46  

#define LCD_H_RES        240
#define LCD_V_RES        320

// Reduzido para 10 linhas para garantir que caiba 100% na memória SRAM interna estável (Evita tela branca)
#define LCD_DRAW_BUFF_SIZE (LCD_H_RES * 10)

void st7789_init(void) {
    ESP_LOGI(TAG, "Configurando pino do Backlight (Forçado)...");
    gpio_reset_pin(PIN_NUM_BK_LIGHT);
    gpio_set_direction(PIN_NUM_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1); // Garante acendimento elétrico dos LEDs

    ESP_LOGI(TAG, "Inicializando Barramento SPI do Display...");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_DRAW_BUFF_SIZE * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Configurando Interface SPI do Driver de Tela...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS, 
        .pclk_hz = 20 * 1000 * 1000,   // 20MHz estável contra capacitância parasitária de cabos
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Instanciando driver nativo ST7789...");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST, 
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // Sequência de reinicialização com intervalos elétricos controlados (Crucial para tirar o ST7789 do congelamento)
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    vTaskDelay(pdMS_TO_TICKS(150)); 
    
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true)); 
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Inicializando a engine do componente LVGL Port...");
    const lvgl_port_cfg_t lvgl_port_cfg = {
        .task_priority = 5,
        .task_stack = 6144, 
        .task_affinity = -1,
        .task_max_sleep_ms = 10
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_port_cfg));

    ESP_LOGI(TAG, "Mapeando display dentro do LVGL...");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_DRAW_BUFF_SIZE,
        .double_buffer = false,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,   // ALTERADO: Desativa buff_dma temporariamente para forçar escrita direta via CPU na RAM
            .buff_spiram = false,
        }
    };
    lv_display_t *lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    
    ESP_LOGI(TAG, "Componente de hardware do display ST7789 e LVGL prontos!");
}
