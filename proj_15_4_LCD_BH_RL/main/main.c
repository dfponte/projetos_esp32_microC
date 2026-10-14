#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "main_app";

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

// --- DECLARAÇÃO DAS TRÊS IMAGENS ---
LV_IMAGE_DECLARE(term_80x80); 
LV_IMAGE_DECLARE(Inte_80x80); 
LV_IMAGE_DECLARE(btn_80x80);  

// Montagem da Interface Gráfica original
void construir_interface(void) {
    static lv_style_t estilo_texto;
    lv_style_init(&estilo_texto);
    lv_style_set_text_font(&estilo_texto, &lv_font_montserrat_22); 
    lv_style_set_text_color(&estilo_texto, lv_color_white());

    // ---- PRIMEIRA IMAGEM (Termômetro) ----
    lv_obj_t * img_termo = lv_image_create(lv_screen_active());
    lv_image_set_src(img_termo, &term_80x80);
    lv_obj_align(img_termo, LV_ALIGN_TOP_LEFT, 10, 15);

    lv_obj_t *label_temp = lv_label_create(lv_screen_active());
    lv_obj_add_style(label_temp, &estilo_texto, 0);
    lv_label_set_text(label_temp, "25 °C");
    lv_obj_align_to(label_temp, img_termo, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // ---- SEGUNDA IMAGEM (Luz / Intensidade) ----
    lv_obj_t * img_luz = lv_image_create(lv_screen_active());
    lv_image_set_src(img_luz, &Inte_80x80);
    lv_obj_align_to(img_luz, img_termo, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

    lv_obj_t *label_lux_val = lv_label_create(lv_screen_active());
    lv_obj_add_style(label_lux_val, &estilo_texto, 0);
    lv_label_set_text(label_lux_val, "100 LUX");
    lv_obj_align_to(label_lux_val, img_luz, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // ---- TERCEIRA IMAGEM (Botão / Relé) ----
    lv_obj_t * img_btn = lv_image_create(lv_screen_active());
    lv_image_set_src(img_btn, &btn_80x80);
    lv_obj_align_to(img_btn, img_luz, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

    lv_obj_t *label_status = lv_label_create(lv_screen_active());
    lv_obj_add_style(label_status, &estilo_texto, 0);
    lv_label_set_text(label_status, "MODO TESTE");
    lv_obj_align_to(label_status, img_btn, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
}

// --- FUNÇÃO PRINCIPAL APP_MAIN ---
void app_main(void) {
    // Configurar pino do Backlight do LCD
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_BK_LIGHT),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1); // Liga backlight

    // Inicializar Barramento SPI do Display
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Interface SPI do Driver de Tela
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = 40 * 1000 * 1000, 
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // Instanciação nativa do driver ST7789 embutido no esp_lcd do IDF
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true); 
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // Inicializa a subestrutura do esp_lvgl_port
    const lvgl_port_cfg_t lvgl_port_cfg = {
        .task_priority = 5,
        .task_stack = 4096,
        .task_affinity = -1,
        .task_max_sleep_ms = 10
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_port_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 20,
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
            .buff_dma = true,
            .buff_spiram = false,
        }
    };
    lv_display_t *lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    (void)lvgl_disp;

    // REMOVIDO O LOCK DE COMPARAÇÃO TIMEOUT: Constrói a interface de forma nativa e direta
    construir_interface();

    ESP_LOGI(TAG, "Objetivo atingido: Display rodando de forma direta e limpa!");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
