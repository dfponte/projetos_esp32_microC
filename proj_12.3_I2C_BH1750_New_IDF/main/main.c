#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "BH1750_S3";

// Configuração de Pinos para o S3
#define I2C_SDA_PIN             8   // Seu pino SDA
#define I2C_SCL_PIN             9   // Sugestão para SCL (comum no S3)
#define BH1750_ADDR             0x23 
#define BH1750_CMD_CONT_HIGH    0x10 

void app_main(void)
{
    // 1. Configurar o barramento I2C Master
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, // Ativa pull-ups internos do S3
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    // 2. Adicionar o BH1750 como dispositivo no barramento
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_ADDR,
        .scl_speed_hz = 100000, // 100kHz (Standard Mode)
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // 3. Comando para iniciar medição contínua
    uint8_t cmd = BH1750_CMD_CONT_HIGH;
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &cmd, 1, -1));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(200)); // Aguarda conversão do sensor

        uint8_t data[2];
        // 4. Receber 2 bytes de dados
        esp_err_t ret = i2c_master_receive(dev_handle, data, 2, -1);

        if (ret == ESP_OK) {
            // Cálculo: (MSB << 8 | LSB) / 1.2
            float lux = (float)((data[0] << 8) | data[1]) / 1.2;
            ESP_LOGI(TAG, "Luminosidade: %.2f Lux", lux);
        } else {
            ESP_LOGE(TAG, "Erro ao ler sensor! Verifique conexões.");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
