#include "driver/i2c.h"
#include "esp_log.h"
#include "bh1750.h"

#define I2C_MASTER_NUM   I2C_NUM_0
#define BH1750_ADDR      0x23 
#define BH1750_CMD_START 0x10 

void bh1750_init(int sda_pin, int scl_pin) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    uint8_t cmd = BH1750_CMD_START;
    i2c_master_write_to_device(I2C_MASTER_NUM, BH1750_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
}

int bh1750_read_lux(void) {
    uint8_t dados[2] = {0, 0}; 
    esp_err_t err = i2c_master_read_from_device(I2C_MASTER_NUM, BH1750_ADDR, dados, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        return -1; 
    }
    uint16_t nivel_bruto = (dados[0] << 8) | dados[1];
    return (int)(nivel_bruto / 1.2);
}
