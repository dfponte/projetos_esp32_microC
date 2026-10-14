#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/i2c.h" 
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

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

// --- CONFIGURAÇÃO DO RELÉ E SENSOR BH1750 ---
#define PIN_RELE         2    
#define I2C_MASTER_NUM   I2C_NUM_0
#define PIN_I2C_SDA      4    
#define PIN_I2C_SCL      5    
#define BH1750_ADDR      0x23 
#define BH1750_CMD_START 0x10 

// --- PONTEIROS GLOBAIS DA INTERFACE ---
static lv_obj_t *label_lux_val = NULL;
static lv_obj_t *card_status = NULL;
static lv_obj_t *label_status = NULL;

static esp_lcd_panel_handle_t panel_handle = NULL;

// Variáveis de controle compartilhadas entre as Tasks
static int g_lux_atual = -1;
static bool g_rele_ligado = false;
static bool g_dados_atualizados = false;

// 1. Função de Flush (Ponte LVGL -> Driver LCD)
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(disp);
}

// 2. Inicialização do Driver I2C para o Sensor BH1750
void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    uint8_t cmd = BH1750_CMD_START;
    i2c_master_write_to_device(I2C_MASTER_NUM, BH1750_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
}

// 3. Função de leitura digital de LUX do BH1750
int ler_bh1750_lux(void) {
    uint8_t dados[2]; 
    esp_err_t err = i2c_master_read_from_device(I2C_MASTER_NUM, BH1750_ADDR, dados, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        return -1; 
    }
    uint16_t nivel_bruto = (dados[0] << 8) | dados[1];
    return (int)(nivel_bruto / 1.2);
}

// 4. CONSTRUÇÃO DA INTERFACE VISUAL ULTRA CLEAN
void construir_interface(void) {
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    label_lux_val = lv_label_create(lv_screen_active());
    lv_label_set_text(label_lux_val, "--- LUX");
    lv_obj_align(label_lux_val, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(label_lux_val, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_lux_val, lv_color_white(), 0);

    card_status = lv_obj_create(lv_screen_active());
    lv_obj_set_size(card_status, 240, 140); 
    lv_obj_align(card_status, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(card_status, 0, 0); 
    lv_obj_set_style_border_width(card_status, 0, 0);
    lv_obj_set_style_bg_color(card_status, lv_color_make(40, 45, 55), 0); 

    label_status = lv_label_create(card_status);
    lv_label_set_text(label_status, "AGUARDANDO SENSOR");
    lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_status, lv_color_white(), 0);
}

// 5. Função de atualização dinâmica (SÓ ATUALIZA SE HOUVER MUDANÇA REAL)
void atualizar_interface(int lux, bool rel_ligado) {
    char buffer[16]; 
    
    if (lux == -1) {
        lv_label_set_text(label_lux_val, "ERRO SENSOR");
        lv_obj_set_style_text_color(label_lux_val, lv_color_make(255, 60, 60), 0);
        
        lv_label_set_text(label_status, "VERIFIQUE FIAÇÃO");
        lv_obj_set_style_bg_color(card_status, lv_color_make(50, 50, 50), 0);
        return;
    }

    snprintf(buffer, sizeof(buffer), "%d LUX", lux);
    lv_label_set_text(label_lux_val, buffer);
    lv_obj_set_style_text_color(label_lux_val, lv_color_white(), 0);

    if (rel_ligado) {
        lv_label_set_text(label_status, "RELAY ON");
        lv_obj_set_style_bg_color(card_status, lv_color_make(210, 40, 40), 0); 
    } else {
        lv_label_set_text(label_status, "RELAY OFF");
        lv_obj_set_style_bg_color(card_status, lv_color_make(30, 150, 70), 0);  
    }
}

// --- TASK SECUNDÁRIA: LEITURA DO SENSOR E CONTROLE DO RELÉ ---
void sensor_relay_task(void *pvParameters) {
    while (1) {
        int lux = ler_bh1750_lux();
        bool acionar_rele = false;

        if (lux >= 0) {
            acionar_rele = (lux < 10); 
            gpio_set_level(PIN_RELE, acionar_rele ? 1 : 0);
        }

        // Se o valor mudou, avisa a Task da LVGL para atualizar a tela
        if (lux != g_lux_atual || acionar_rele != g_rele_ligado) {
            g_lux_atual = lux;
            g_rele_ligado = acionar_rele;
            g_dados_atualizados = true; // Flag sinalizando mudança
        }

        // Lê o sensor a cada 500ms (tempo perfeito para o BH1750 trabalhar folgado)
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// --- FUNÇÃO PRINCIPAL APP_MAIN ---
void app_main(void) {
    // Inicializar Barramento SPI do Display
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Interface SPI
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

    // Driver ST7789
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true); 
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // Ligar Luz de Fundo
    gpio_set_direction(PIN_NUM_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);

    // Configurar pino do Relé
    gpio_set_direction(PIN_RELE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RELE, 0);

    // Inicializar I2C para o Sensor BH1750
    i2c_master_init();
    vTaskDelay(pdMS_TO_TICKS(150));

    // Configurar Engine da LVGL v9
    lv_init();
    lv_display_t * disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);

    static lv_color_t buf1[LCD_H_RES * 32];
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Renderiza a interface limpa
    construir_interface();

    // Cria a Task do Sensor no Core 1 para não interferir na animação da tela (Core 0)
    xTaskCreatePinnedToCore(sensor_relay_task, "sensor_task", 4096, NULL, 5, NULL, 1);

    // Loop Principal: Focado 100% no desempenho da Interface Gráfica
    while (1) {
        // Só redesenha a tela se a Task do sensor tiver enviado dados novos
        if (g_dados_atualizados) {
            atualizar_interface(g_lux_atual, g_rele_ligado);
            g_dados_atualizados = false; // Reseta o gatilho
        }

        lv_tick_inc(20);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
