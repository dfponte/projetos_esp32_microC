#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#define I2C_PORT            I2C_NUM_0
#define ADS1115_ADDR        0x48   
#define LSB_SIZE_6V         0.0001875f // Valor de 1 bit para escala 6.144V

int16_t read_ads_6v(uint8_t channel) {
    // Config: 
    // Bit 15: 1 (Start)
    // Bits 14-12: Canal
    // Bits 11-9: 000 (FSR = +/- 6.144V) <--- MUDANÇA AQUI
    // Bits 8: 1 (Single-shot)
    uint16_t config = 0x8083; 
    config |= ((uint16_t)(0x04 | channel) << 12);

    uint8_t out[3];
    out[0] = 0x01;                   // Ponteiro Config
    out[1] = (uint8_t)(config >> 8);   // MSB
    out[2] = (uint8_t)(config & 0xFF); // LSB

    i2c_master_write_to_device(I2C_PORT, ADS1115_ADDR, out, 3, pdMS_TO_TICKS(50));
    
    // Tempo para conversão física
    vTaskDelay(pdMS_TO_TICKS(20)); 

    uint8_t reg_ptr = 0x00;
    uint8_t data[2];
    i2c_master_write_read_device(I2C_PORT, ADS1115_ADDR, &reg_ptr, 1, data, 2, pdMS_TO_TICKS(50));

    return (int16_t)((data[0] << 8) | data[1]);
}

void app_main(void) {
    // Inicialização básica (SCL=1, SDA=2)
    i2c_config_t conf = { .mode = I2C_MODE_MASTER, .sda_io_num = 2, .scl_io_num = 1, 
                          .sda_pullup_en = 1, .scl_pullup_en = 1, .master.clk_speed = 100000 };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);

    while (1) {
        printf("\n--- LEITURA ESCALA 6.144V ---\n");
        for (int i = 0; i < 4; i++) {
            int16_t raw = read_ads_6v(i);
            float voltage = raw * LSB_SIZE_6V;
            printf("CH%d: %d | %.3f V  ", i, raw, voltage);
            if (i % 2 != 0) printf("\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
