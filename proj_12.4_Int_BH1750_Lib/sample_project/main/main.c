#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "bh1750.h"

// Defina os pinos manualmente para o ESP32-S3
#define SDA_GPIO 8
#define SCL_GPIO 9
#define I2C_PORT 0

void test_task(void *pvParameters)
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    // BH1750_ADDR_LO é o endereço padrão 0x23
    ESP_ERROR_CHECK(bh1750_init_desc(&dev, BH1750_ADDR_LO, I2C_PORT, SDA_GPIO, SCL_GPIO));
    ESP_ERROR_CHECK(bh1750_setup(&dev, BH1750_MODE_CONTINUOUS, BH1750_RES_HIGH));

    while (1)
    {
        uint16_t lux;
        if (bh1750_read(&dev, &lux) != ESP_OK) {
            printf("Erro na leitura do sensor!\n");
        } else {
            printf("Luminosidade: %d lux\n", lux);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main()
{
    // Inicializa a biblioteca de abstração I2C do esp-idf-lib
    ESP_ERROR_CHECK(i2cdev_init()); 

    // Cria a tarefa de leitura
    xTaskCreatePinnedToCore(test_task, "bh1750_test", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL, 0);
}
