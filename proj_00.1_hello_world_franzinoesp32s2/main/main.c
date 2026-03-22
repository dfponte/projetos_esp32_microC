

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "LED"; 

//Configuração do pino 2

 gpio_config_t config_pi = {
.pin_bit_mask = (1ULL << GPIO_NUM_21),
.mode = GPIO_MODE_OUTPUT,
.pull_up_en = GPIO_PULLUP_DISABLE,
.pull_down_en = GPIO_PULLDOWN_ENABLE,
.intr_type = GPIO_INTR_DISABLE
 };


void app_main() {

 //Realiza a configuração dos pinos
   ESP_ERROR_CHECK(gpio_config(&config_pi));

   while (1)
   {
    /* code */
    gpio_set_level(GPIO_NUM_21,0);
    printf("Led Off");
    ESP_LOGI(TAG,"DESLIGADO");
    vTaskDelay(1000/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_21,1);
    printf("Led ON");
    ESP_LOGI(TAG,"LIGADO");
    vTaskDelay(1000/portTICK_PERIOD_MS);
   }


}