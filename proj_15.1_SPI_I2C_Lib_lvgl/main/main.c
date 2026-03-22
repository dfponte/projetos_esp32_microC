#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

// --- CONFIGURAÇÃO DE PINOS ESP32-S3 ---
#define LCD_HOST        SPI2_HOST
#define PIN_NUM_SCLK    12
#define PIN_NUM_MOSI    11
#define PIN_NUM_LCD_DC  9
#define PIN_NUM_LCD_CS  10
#define PIN_NUM_LCD_RST 3
#define PIN_NUM_BK_LIGHT 46

#define LCD_H_RES       240
#define LCD_V_RES       320

// Handle global para o painel (necessário para a função de flush do LVGL)
static esp_lcd_panel_handle_t panel_handle = NULL;

// 1. Função de "Ponte" (Flush Callback) entre LVGL e o Driver de LCD
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    // Envia os pixels calculados pelo LVGL para o hardware do ST7789
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    
    // Avisa o LVGL que a transferência terminou
    lv_display_flush_ready(disp);
}

void app_main(void) {
    // 2. INICIALIZAR BARRAMENTO SPI
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 3. CONFIGURAR INTERFACE SPI DO PAINEL
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = 40 * 1000 * 1000, // 40MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // 4. INSTALAR DRIVER ST7789
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // Inicialização física
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true); // Necessário para a maioria dos displays 2.0"
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // Ligar Backlight
    gpio_set_direction(PIN_NUM_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);

    // 5. CONFIGURAR LVGL
    lv_init();
    lv_display_t * disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);

    // Criar Buffer de desenho (1/10 da tela para economizar RAM interna)
    static lv_color_t buf1[LCD_H_RES * 32];
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 6. CRIAR INTERFACE (Label e Barra de Progresso)
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "ESP32-S3 + LVGL 9");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14,3);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);
    //lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 40);
    //lv_obj_center(label); 
    lv_obj_t * bar = lv_bar_create(lv_screen_active());
    lv_obj_set_size(bar, 180, 20);
    lv_obj_center(bar);
    lv_bar_set_value(bar, 75, LV_ANIM_ON);

    // 7. LOOP PRINCIPAL DO LVGL
    int progress = 0;
    while (1) {
        // Atualiza a barra de progresso para ver movimento
        progress = (progress + 1) % 101;
        lv_bar_set_value(bar, progress, LV_ANIM_OFF);

        // O LVGL precisa ser avisado da passagem do tempo e processar tarefas
        lv_tick_inc(10);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
