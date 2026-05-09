#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "vl53l0x.h"

static const char *TAG = "VL53L0X";

// Pinos para o seu ESP32-S3
#define SDA_PIN 8
#define SCL_PIN 9
#define XSHUT_PIN -1 // Use -1 se não estiver usando o pino de desligamento
#define I2C_PORT 0

void app_main() {
    // 1. O seu driver cria a estrutura e configura o I2C internamente
    // vl53l0x_config(porta, scl, sda, xshut, endereco, io_2v8)
    vl53l0x_t *dev = vl53l0x_config(I2C_PORT, SCL_PIN, SDA_PIN, XSHUT_PIN, 0x29, 1);

    if (dev == NULL) {
        ESP_LOGE(TAG, "Erro ao criar a estrutura do sensor!");
        return;
    }

    // 2. Inicializa o hardware
    const char *error_msg = vl53l0x_init(dev);
    // Aumenta o orçamento de tempo para 200ms (mais precisão)
      vl53l0x_setMeasurementTimingBudget(dev, 200000); 

    if (error_msg != NULL) {
        ESP_LOGE(TAG, "Erro na inicializacao: %s", error_msg);
        return;
    }

    ESP_LOGI(TAG, "Sensor inicializado com sucesso!");

    while (1) {
        // 3. Usa a função de leitura que está no seu .h
        uint16_t distance = vl53l0x_readRangeSingleMillimeters(dev);

        if (vl53l0x_timeoutOccurred(dev)) {
            ESP_LOGE(TAG, "Timeout na leitura!");
        } else {
            printf("Distancia: %d mm\n", distance);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
