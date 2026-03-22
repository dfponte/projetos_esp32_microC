#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

// Definimos o pino do LED, que pode ser configurado via idf.py menuconfig
// ou definido diretamente aqui. Usaremos o GPIO 2 como padrão.
#define BLINK_GPIO CONFIG_BLINK_GPIO

static const char *TAG = "ETX_BLINKY"; // Para mensagens de log

void app_main(void)
{
    // Reset o pino antes de configurar
    gpio_reset_pin(BLINK_GPIO);

    // Configura o pino GPIO como saída
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "GPIO %d configurado como saída", BLINK_GPIO);

    while (1) {
        // Liga o LED (nível lógico alto = 1)
        printf("Config WIFI SSID : %s \n",CONFIG_wifi_ssid);
        printf("Senha rede : %s \n",CONFIG_wifi_password);
        gpio_set_level(BLINK_GPIO, 1);
        ESP_LOGI(TAG, "LED LIGADO");
        // Atraso de 1 segundo (1000 ms)
        vTaskDelay(1000 / portTICK_PERIOD_MS); 

        // Desliga o LED (nível lógico baixo = 0)
        gpio_set_level(BLINK_GPIO, 0);
        ESP_LOGI(TAG, "LED DESLIGADO");
        // Atraso de mais 1 segundo (1000 ms)
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}