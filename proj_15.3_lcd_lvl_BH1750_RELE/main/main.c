#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "esp_log.h"
#include "st7789.h" 

static const char *TAG = "main_app";

// --- REFERÊNCIAS DOS PINOS PARA EVITAR FLUTUAÇÃO ELÉTRICA ---
#define PIN_RELE         2    
#define PIN_I2C_SDA      4    
#define PIN_I2C_SCL      5    

// --- PONTEIRO GLOBAL PARA O VALOR DINÂMICO ---
static lv_obj_t *label_valor_dinamico = NULL;

// --- DECLARAÇÃO DAS TRÊS IMAGENS ---
extern const lv_image_dsc_t term_80x80; 
extern const lv_image_dsc_t Inte_80x80; 
extern const lv_image_dsc_t btn_80x80;  

// Montagem da Interface Gráfica na Vertical
void construir_interface(void) {
    static lv_style_t estilo_texto;
    lv_style_init(&estilo_texto);
    lv_style_set_text_font(&estilo_texto, &lv_font_montserrat_22); 
    lv_style_set_text_color(&estilo_texto, lv_color_white());

    // ---- PRIMEIRA IMAGEM (Termômetro) ----
    lv_obj_t * img_termo = lv_image_create(lv_screen_active());
    lv_image_set_src(img_termo, &term_80x80);
    lv_obj_set_style_image_opa(img_termo, LV_OPA_COVER, 0); 
    lv_obj_align(img_termo, LV_ALIGN_TOP_LEFT, 10, 15);

    lv_obj_t *label_temp = lv_label_create(lv_screen_active());
    lv_obj_add_style(label_temp, &estilo_texto, 0);
    lv_label_set_text(label_temp, "25 °C");
    lv_obj_align_to(label_temp, img_termo, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // ---- SEGUNDA IMAGEM (Luz / Intensidade) ----
    lv_obj_t * img_luz = lv_image_create(lv_screen_active());
    lv_image_set_src(img_luz, &Inte_80x80);
    lv_obj_set_style_image_opa(img_luz, LV_OPA_COVER, 0);
    lv_obj_align_to(img_luz, img_termo, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

    // Texto dinâmico que receberá a simulação
    label_valor_dinamico = lv_label_create(lv_screen_active());
    lv_obj_add_style(label_valor_dinamico, &estilo_texto, 0);
    lv_label_set_text(label_valor_dinamico, "--- LUX");
    lv_obj_align_to(label_valor_dinamico, img_luz, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // ---- TERCEIRA IMAGEM (Botão / Relé) ----
    lv_obj_t * img_btn = lv_image_create(lv_screen_active());
    lv_image_set_src(img_btn, &btn_80x80);
    lv_obj_set_style_image_opa(img_btn, LV_OPA_COVER, 0);
    lv_obj_align_to(img_btn, img_luz, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

    lv_obj_t *label_status = lv_label_create(lv_screen_active());
    lv_obj_add_style(label_status, &estilo_texto, 0);
    lv_label_set_text(label_status, "PRONTO");
    lv_obj_set_style_text_color(label_status, lv_color_make(60, 255, 100), 0); 
    lv_obj_align_to(label_status, img_btn, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
}

// --- FUNÇÃO PRINCIPAL APP_MAIN ---
void app_main(void) {
    // ESTABILIZAÇÃO ELÉTRICA: Configura e aterra os pinos sobressalentes para evitar flutuação
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_RELE) | (1ULL << PIN_I2C_SDA) | (1ULL << PIN_I2C_SCL),
        .pull_down_en = GPIO_PULLDOWN_ENABLE, // Trava em nível lógico estável
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_RELE, 0); 
    gpio_set_level(PIN_I2C_SDA, 0);
    gpio_set_level(PIN_I2C_SCL, 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Inicializa o componente separado do Display ST7789
    st7789_init();

    // Monta a interface na tela de forma segura usando travas (Locks)
    lvgl_port_lock(0);
    construir_interface();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Interface vertical ativa. Iniciando loop de simulação.");

    float simulacao_lux = 15.0;
    char buffer_texto[32];

    while (1) {
        simulacao_lux += 2.5;
        if (simulacao_lux > 120.0) {
            simulacao_lux = 15.0;
        }

        // Atualiza o texto do valor dinâmico com proteção contra concorrência
        lvgl_port_lock(0);
        snprintf(buffer_texto, sizeof(buffer_texto), "%.1f LUX", simulacao_lux);
        lv_label_set_text(label_valor_dinamico, buffer_texto);
        lvgl_port_unlock();

        vTaskDelay(pdMS_TO_TICKS(1000)); // Atualiza o valor a cada 1 segundo
    }
}
