
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2cdev.h"
#include "mpu6050.h"

static const char *TAG = "MPU6050_S3";

// Forçando os pinos 9 e 8
#define PIN_SDA 8
#define PIN_SCL 9

void mpu_task(void *pvParameters) {
    // Declarando a estrutura de forma explícita
    mpu6050_dev_t dev;
    memset(&dev, 0, sizeof(mpu6050_dev_t));

    // Inicializa o descritor (Ponteiro, Endereço, Porta, SDA, SCL)
    // O cast (gpio_num_t) remove o erro de "incompatible pointer/type"
    esp_err_t ret = mpu6050_init_desc(&dev, 0x68, 0, (gpio_num_t)PIN_SDA, (gpio_num_t)PIN_SCL);
    
    // Verifique ou force a frequência para 100.000 Hz (100kHz)
      dev.i2c_dev.cfg.master.clk_speed = 100000; 

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro init_desc: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    // Inicializa o sensor (acorda do sleep)
    ret = mpu6050_init(&dev);
    uint8_t wake_cmd = 0;
    i2c_dev_write_reg(&dev.i2c_dev, 0x6B, &wake_cmd, 1);
    vTaskDelay(pdMS_TO_TICKS(100)); // Aguarda estabilizar
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro mpu6050_init: %s. Verifique fiação!", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }
     
      mpu6050_set_full_scale_accel_range(&dev, MPU6050_ACCEL_RANGE_2);
     // mpu6050_set_gyro_full_scale(&dev, MPU6050_GYRO_RANGE_250);
// 3. Define a escala manualmente (evita que fique em modo indefinido)
   
    //mpu6050_acceleration_t acc;
    mpu6050_acceleration_t acc;
    mpu6050_rotation_t gyro;
    float temp; // Temperatura geralmente é retornada como float

    //---------verificar ---------------

    uint8_t id = 0;
  // Lê o registrador 0x75 (Who Am I)
    esp_err_t id_ret = i2c_dev_read_reg(&dev.i2c_dev, 0x75, &id, 1);

      if (id_ret == ESP_OK) {
      printf("Conexão I2C OK! ID do sensor: 0x%02X\n", id);
       if (id != 0x68) {
        printf("AVISO: O ID deveria ser 0x68. Verifique o modelo do sensor.\n");
       }
        } else {
       printf("ERRO FÍSICO: O ESP32 não consegue falar com o sensor nos pinos 8 e 9: %s\n", esp_err_to_name(id_ret));
       }
    /**
     * @brief 
     * while (1)
    {
        // Passando o ponteiro &dev para a leitura
        if (mpu6050_get_acceleration(&dev, &acc) == ESP_OK)
        {
            printf("ACCEL -> X: %.2f  Y: %.2f  Z: %.2f\n", acc.x, acc.y, acc.z);
        }
        else
        {
            ESP_LOGE(TAG, "Erro na leitura!");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
     * 
     */
//***********************************************/
    while (1) {
        // Esta função lê os 6 eixos + temperatura em uma única transação I2C
       // esp_err_t ret = mpu6050_get_motion(&dev, &acc, &gyro);
       
        esp_err_t ret = mpu6050_get_acceleration(&dev, &acc);
        if (ret == ESP_OK) {
            // Acessando os três sinais de cada sensor
              int16_t raw_x = acc.x;
              int16_t raw_y = acc.y;
              int16_t raw_z = acc.z;
           //  printf("ACC: X=%f Y=%f Z=%f\n", acc.x, acc.y, acc.z);
            printf("ACC: X=%d Y=%d Z=%d\n",raw_x,raw_y,raw_z);
            printf("Erro I2C: %s\n", esp_err_to_name(ret));
            
            //printf("GYRO: X=%f Y=%f Z=%f\n", gyro.x, gyro.y, gyro.z);
            
            // Se precisar da temperatura também:
           // mpu6050_get_temperature(&dev, &temp);
            //printf("TEMP: %.2f °C\n", temp);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
    //********************************************** */
}

void app_main(void) {
    // PASSO OBRIGATÓRIO: Inicializa o barramento interno da biblioteca
    ESP_ERROR_CHECK(i2cdev_init());

    xTaskCreate(mpu_task, "mpu_task", 4096, NULL, 5, NULL);
}
