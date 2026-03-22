#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2cdev.h"
#include "mpu6050.h"

#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define I2C_PORT 0

void calibrar_mpu(mpu6050_dev_t *dev, int16_t *off_x, int16_t *off_y, int16_t *off_z);

int16_t offset_x, offset_y, offset_z;

void mpu_task(void *pvParameters) {
    mpu6050_dev_t dev = { 0 };
    mpu6050_acceleration_t acc;
    mpu6050_rotation_t gyro;

    i2cdev_init();
    mpu6050_init_desc(&dev,0x68, I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN);

    // --- PULO DO GATO: ACORDAR O CHIP 0x70 MANUALMENTE ---
    // O erro "Invalid device address" acontece dentro do mpu6050_init().
    // Vamos pular ele e escrever direto no registrador de energia (0x6B).
    uint8_t pwr_mgmt_1 = 0x01; // 0x01 acorda o chip e define o clock estável
    i2c_dev_write_reg(&dev.i2c_dev, 0x6B, &pwr_mgmt_1, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Configura a escala (Se a função der erro de ID, use escrita direta)
    uint8_t accel_config = 0x00; // ± 2g
   
     // Chama a calibração UMA VEZ no início
    
    while (1) {
         i2c_dev_write_reg(&dev.i2c_dev, 0x1C, &accel_config, 1);
        // Tentamos ler usando a biblioteca. Se ela bloquear pelo ID,
        // os valores de acc.x etc. virão do mpu6050_get_motion
        if (mpu6050_get_motion(&dev, &acc, &gyro) == ESP_OK) {
            calibrar_mpu(&dev, &offset_x, &offset_y, &offset_z);
            int16_t raw_x = (int16_t)(acc.x * 16384)-offset_x;
            int16_t raw_y = (int16_t)(acc.y * 16384)-offset_y;
            int16_t raw_z = (int16_t)(acc.z * 16384)-offset_z;
            // Se o compilador pede %f, fazemos o cast para float
            //printf("ACC -> X:%.2f | Y:%.2f | Z:%.2f\n", (float)acc.x, (float)acc.y, (float)acc.z);
            printf("ACC -> X:%d | Y:%d | Z:%d\n",raw_x,raw_y,raw_z);
            
        } 
         else {
            // Se a biblioteca se recusar a ler, o erro aparecerá aqui
            printf("Erro de leitura: Sensor 0x70 bloqueado pela lib.\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main() {
    xTaskCreate(mpu_task, "mpu_task", 4096, NULL, 5, NULL);
}

//-----------------------------------------

void calibrar_mpu(mpu6050_dev_t *dev, int16_t *off_x, int16_t *off_y, int16_t *off_z) {
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    mpu6050_acceleration_t acc;
    int amostras = 100;

    printf("Calibrando... Mantenha o sensor PARADO e NIVELADO!\n");
    for (int i = 0; i < amostras; i++) {
        if (mpu6050_get_acceleration(dev, &acc) == ESP_OK) {
            sum_x += (int16_t)(acc.x * 16384);
            sum_y += (int16_t)(acc.y * 16384);
            sum_z += (int16_t)(acc.z * 16384);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    *off_x = sum_x / amostras;
    *off_y = sum_y / amostras;
    // No eixo Z, o erro é a diferença para 1g (16384 na escala de 2g)
    *off_z = (sum_z / amostras) - 16384; 
    
    printf("Offsets calculados -> X:%d Y:%d Z:%d\n", *off_x, *off_y, *off_z);
}
