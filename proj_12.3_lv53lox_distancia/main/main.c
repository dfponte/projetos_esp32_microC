#include "vl53l0x.h" 
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"


static const char *TAG = "VL53L0X_S3";

#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema...");

    // 1. Configuração do barramento I2C (Necessário para o hardware)
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    // 2. Usar ponteiro para evitar erro de "storage size"
    // O driver deve alocar ou gerenciar essa memória internamente
    //vl53l0x_t *sensor = NULL; 
     
   vl53l0x_t *sensor = malloc(sizeof(vl53l0x_t)); 
     //vl53l0x_t sensor; // Declarado como variável (aloca memória)

     
     const char *err = vl53l0x_init(sensor); 
     

    // 3. Inicialização
    // Passamos o endereço do ponteiro. 
    // Se a função vl53l0x_init esperar apenas (vl53l0x_t *), tente passar (sensor)
    //const char *status = vl53l0x_init(sensor);
   
    if (err!=NULL) {
        printf("Falha ao inicializar o sensor!\n");
        ESP_LOGE(TAG, "Erro: %s", err);
    } else {
        ESP_LOGI(TAG, "VL53L0X inicializado!");
    }

    // 4. Loop de Leitura
    while (1) {
        // Ajuste o nome da função de leitura conforme seu .h
        uint16_t d = vl53l0x_readRangeSingleMillimeters(sensor);
        
        if (d < 8000) {
            ESP_LOGI(TAG, "Distancia: %d mm", d);
        }
    
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
