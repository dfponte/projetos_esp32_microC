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

// Variáveis para evitar atualizações redundantes e flickering na LVGL
static int g_ultimo_lux = -999;
static int g_ultimo_rele = -1;

// 1. Inicialização do Driver I2C para o Sensor BH1750
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

// 2. Leitura digital de LUX do BH1750
int ler_bh1750_lux(void) {
    uint8_t dados[2] = {0, 0}; 
    esp_err_t err = i2c_master_read_from_device(I2C_MASTER_NUM, BH1750_ADDR, dados, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        return -1; 
    }
    uint16_t nivel_bruto = (dados[0] << 8) | dados[1];
    return (int)(nivel_bruto / 1.2);
}

// 3. CONSTRUÇÃO DA INTERFACE VISUAL ULTRA CLEAN (Deve rodar sob Lock)
void construir_interface(void) {
    // Fundo Preto Absoluto para Contraste Máximo
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    // Bloco Superior: Texto com o Valor em LUX
    label_lux_val = lv_label_create(lv_screen_active());
    lv_label_set_text(label_lux_val, "--- LUX");
    lv_obj_align(label_lux_val, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(label_lux_val, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_lux_val, lv_color_white(), 0);

    // Bloco Inferior: Painel de Status do Relé
    card_status = lv_obj_create(lv_screen_active());
    lv_obj_set_size(card_status, 240, 140); 
    lv_obj_align(card_status, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(card_status, 0, 0); 
    lv_obj_set_style_border_width(card_status, 0, 0);
    lv_obj_set_style_bg_color(card_status, lv_color_make(40, 45, 55), 0); 

    // Texto de Status Centralizado dentro do bloco inferior
    label_status = lv_label_create(card_status);
    lv_label_set_text(label_status, "AGUARDANDO SENSOR");
    lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_status, lv_color_white(), 0);
}

// 4. Função de atualização dinâmica controlada (Deve rodar sob Lock)
void atualizar_interface(int lux, bool rel_ligado) {
    char buffer[32]; 
    
    if (lux == -1) {
        if (g_ultimo_lux != -1) {
            lv_label_set_text(label_lux_val, "ERRO SENSOR");
            lv_obj_set_style_text_color(label_lux_val, lv_color_make(255, 60, 60), 0);
            lv_label_set_text(label_status, "VERIFIQUE FIAÇÃO");
            lv_obj_set_style_bg_color(card_status, lv_color_make(50, 50, 50), 0);
            g_ultimo_lux = -1;
            g_ultimo_rele = -1;
        }
        return;
    }

    // Só atualiza o texto de LUX se ele mudar de valor
    if (lux != g_ultimo_lux) {
        snprintf(buffer, sizeof(buffer), "%d LUX", lux);
        lv_label_set_text(label_lux_val, buffer);
        lv_obj_set_style_text_color(label_lux_val, lv_color_white(), 0);
        g_ultimo_lux = lux;
    }

    // Só redesenha o bloco do relé se o estado lógico mudar
    if ((int)rel_ligado != g_ultimo_rele) {
        if (rel_ligado) {
            lv_label_set_text(label_status, "RELAY ON");
            lv_obj_set_style_bg_color(card_status, lv_color_make(210, 40, 40), 0); // Vermelho
        } else {
            lv_label_set_text(label_status, "RELAY OFF");
            lv_obj_set_style_bg_color(card_status, lv_color_make(30, 150, 70), 0);  // Verde
        }
        g_ultimo_rele = (int)rel_ligado;
    }
}

// --- FUNÇÃO PRINCIPAL APP_MAIN ---
void app_main(void) {
    // Configurar pino do Relé e Backlight do LCD
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_RELE) | (1ULL << PIN_NUM_BK_LIGHT),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1); // Liga backlight
    gpio_set_level(PIN_RELE, 0);         // Inicia relé desligado

    // Inicializar I2C para o Sensor BH1750
    i2c_master_init();
    vTaskDelay(pdMS_TO_TICKS(100));

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
        .pclk_hz = 40 * 1000 * 1000, // 40MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // Instanciação nativa do driver ST7789 embutido no esp_lcd do IDF v5.5
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true); // Corrige inversões cromáticas comuns em telas IPS
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // Inicializa a engine do componente LVGL Port
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 40,
        .double_buffer = false,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags = { .buff_dma = true }
    };
 
    // Adicionado o cast explícito para remover o erro de conversão
   
   lv_display_t *display = (lv_display_t *)lvgl_port_add_disp(&disp_cfg);
    (void)display; // Mantém para evitar aviso de variável não utilizada

    // Protege a criação dos elementos visuais iniciais
    if (lvgl_port_lock(0)) {
        construir_interface();
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "Loop de monitoramento iniciado.");
    
    // Loop principal: O sensor é lido a cada 300ms de maneira segura
    while (1) {
        int lux_atual = ler_bh1750_lux();
        bool acionar_rele = false;

        if (lux_atual >= 0) {
            acionar_rele = (lux_atual < 10); 
            gpio_set_level(PIN_RELE, acionar_rele ? 1 : 0);
        }

        // Bloqueia temporariamente o thread gráfico para atualizar as labels com segurança
        if (lvgl_port_lock(0)) {
            atualizar_interface(lux_atual, acionar_rele);
            lvgl_port_unlock(); // Libera imediatamente a renderização da tela
        }

        vTaskDelay(pdMS_TO_TICKS(300)); 
    }
}
