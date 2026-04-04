#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h>

#define I2C_MASTER_SCL_IO           9      // Pino SCL do ESP32-S3
#define I2C_MASTER_SDA_IO           8      // Pino SDA do ESP32-S3
#define I2C_MASTER_NUM              0      
#define BH1750_SENSOR_ADDR          0x23   // Endereço I2C (ADDR no GND)
#define BH1750_CMD_START            0x10   // Modo Continuo Alta Resolução

void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void app_main(void) {
    i2c_master_init();

    // 1. Envia comando para ligar e configurar modo do sensor
    uint8_t cmd = BH1750_CMD_START;
    i2c_master_write_to_device(I2C_MASTER_NUM, BH1750_SENSOR_ADDR, &cmd, 1, pdMS_TO_TICKS(1000));

    while (1) {
        uint8_t data[2];
        // 2. Lê 2 bytes de luminosidade
        esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, BH1750_SENSOR_ADDR, data, 2, pdMS_TO_TICKS(1000));

        if (ret == ESP_OK) {
            // 3. Converte os bytes para LUX (Fórmula: (MSB << 8 | LSB) / 1.2)
            float lux = (float)((data[0] << 8) | data[1]) / 1.2;
            printf("Luminosidade: %.2f lx\n", lux);
        } else {
            printf("Erro na leitura I2C\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}