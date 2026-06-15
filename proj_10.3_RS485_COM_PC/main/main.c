#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"      
#include "driver/uart.h"      
#include "esp_modbus_common.h"
#include "esp_modbus_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MB_UART_PORT_NUM      (UART_NUM_2)       
#define MB_BAUD_RATE          (9600)             
#define MB_UART_TXD_PIN       (GPIO_NUM_17)      
#define MB_UART_RXD_PIN       (GPIO_NUM_18)      
#define MB_UART_RTS_PIN       (GPIO_NUM_19)      
#define MB_DEVICE_ADDR1       (1) 

static const char *TAG = "MASTER_MODBUS";

enum {
    CID_HOLD_REG_0 = 0,
    CID_COUNT
};

// Dicionário de dados compatível com esp-modbus v2.x
const mb_parameter_descriptor_t device_parameters[] = {
    { 
        CID_HOLD_REG_0,              
        "Holding_Reg_0",            
        "Volts",                    
        MB_DEVICE_ADDR1,            
        MB_PARAM_HOLDING,           
        0,                          
        1,                          
        0,                          
        PARAM_TYPE_U16,             
        2,                          
        { .opt1 = 0 },              
        PAR_PERMS_READ_WRITE_TRIGGER 
    }
};
const uint16_t num_device_parameters = (sizeof(device_parameters) / sizeof(device_parameters));

void app_main(void)
{
    void* master_handle = NULL;

    // 1. Inicializa o driver (Versão v2.1.2 exige a passagem do ponteiro master_handle)
    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handle);
    if (err != ESP_OK) {
        printf("Falha ao inicializar mbc_master_init: %s\n", esp_err_to_name(err));
        return;
    }

    // 2. Montagem da estrutura isolando a atribuição para evitar falhas em uniões anônimas
    mb_communication_info_t comm_info;
    memset(&comm_info, 0, sizeof(mb_communication_info_t));
    
    comm_info.mode = MB_MODE_RTU; 
    comm_info.serial.port = MB_UART_PORT_NUM;
    comm_info.serial.baudrate = MB_BAUD_RATE;
    comm_info.serial.parity = MB_PARITY_NONE;

    // 3. Configuração dos parâmetros de hardware (Exige master_handle na versão v2.x)
    ESP_ERROR_CHECK(mbc_master_setup(master_handle, &comm_info));

    // 4. Definição física de pinos e controle automático de RS485 Half Duplex
    ESP_ERROR_CHECK(uart_set_pin(MB_UART_PORT_NUM, MB_UART_TXD_PIN, MB_UART_RXD_PIN, MB_UART_RTS_PIN, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(MB_UART_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));

    // 5. Registro do dicionário de parâmetros (Exige master_handle na versão v2.x)
    ESP_ERROR_CHECK(mbc_master_set_descriptor(master_handle, device_parameters, num_device_parameters));

    // 6. Inicialização da máquina de estados do Modbus (Exige master_handle na versão v2.x)
    ESP_ERROR_CHECK(mbc_master_start(master_handle));

    ESP_LOGI(TAG, "Modbus Master inicializado com sucesso em modo RTU!");

    uint16_t reg_value = 0;
    uint8_t param_type = 0; 

    while (1) {
        // 7. Chamada de leitura contendo os 5 argumentos oficiais exigidos pelo ecossistema estável v2.1.2 [^1.5.6]
        err = mbc_master_get_parameter(master_handle, CID_HOLD_REG_0, (char*)"Holding_Reg_0", (uint8_t*)&reg_value, &param_type);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Valor lido com sucesso: %d", reg_value);
        } else {
            ESP_LOGE(TAG, "Falha na leitura do parametro: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
