#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h" // Inclui a biblioteca de log

static const char *TAG = "SERVO_CONTROL"; // Identificador para o log

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_PULSE_PERIOD_US   20000

    uint32_t angle_to_duty(int angle) {
    uint32_t pulsewidth = SERVO_MIN_PULSEWIDTH_US + (angle * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / 180);
    return (pulsewidth * 8192) / SERVO_PULSE_PERIOD_US;
}

void set_servo_angle(int angle) {
    ESP_LOGI(TAG, "Movendo para o angulo: %d graus", angle); // Mensagem informativa (INFO)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, angle_to_duty(angle));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void app_main(void) {
    ESP_LOGI(TAG, "Inicializando PWM para o Servo...");

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 50,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .gpio_num       = 18,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    while (1) {
        set_servo_angle(180);
        vTaskDelay(pdMS_TO_TICKS(550));

        
        vTaskDelay(pdMS_TO_TICKS(2000));

        //set_servo_angle(180);
        //vTaskDelay(pdMS_TO_TICKS(2000));

       // set_servo_angle(270);
        //vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
