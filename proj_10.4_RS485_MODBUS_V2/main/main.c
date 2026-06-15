#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"      
#include "driver/uart.h"      
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Definições de Configuração da UART e Pinos RS485
#define MB_UART_PORT_NUM      (UART_NUM_2)       
#define MB_BAUD_RATE          (9600)             
#define MB_UART_TXD_PIN       (GPIO_NUM_17)      
#define MB_UART_RXD_PIN       (GPIO_NUM_18)      
#define MB_UART_RTS_PIN       (GPIO_NUM_19)      
#define MB_DEVICE_ADDR1       (1) 

static const char *TAG = "MASTER_MODBUS";

// Estrutura clássica de um pacote de leitura Modbus RTU (Função 03 - Read Holding Registers)
// Usar comandos brutos via UART elimina 100% os bugs de compatibilidade de versão do esp-modbus
void enviar_requisicao_modbus_read_reg0(void)
{
    // Frame Modbus RTU: [Slave ID] [Function] [Starting Addr High] [Starting Addr Low] [No. of Reg High] [No. of Reg Low] [CRC Low] [CRC High]
    uint8_t frame_leitura[8] = { MB_DEVICE_ADDR1, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A };
    
    // Envia o pacote de bytes diretamente pelo hardware da UART configurada para RS485
    uart_write_bytes(MB_UART_PORT_NUM, (const char*)frame_leitura, sizeof(frame_leitura));
}

void app_main(void)
{
    // 1. Configuração padrão do driver de hardware UART para a rede RS485
    uart_config_t uart_config = {
        .baud_rate = MB_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 2. Inicializa o driver da UART instalando o buffer de recepção
    ESP_ERROR_CHECK(uart_driver_install(MB_UART_PORT_NUM, 512, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MB_UART_PORT_NUM, &uart_config));

    // 3. Define os pinos físicos TX, RX e RTS do chip transceiver RS485
    ESP_ERROR_CHECK(uart_set_pin(MB_UART_PORT_NUM, MB_UART_TXD_PIN, MB_UART_RXD_PIN, MB_UART_RTS_PIN, UART_PIN_NO_CHANGE));

    // 4. Ativa o modo RS485 Half Duplex (O ESP32 chaveará o pino RTS automaticamente para transmitir/receber)
    ESP_ERROR_CHECK(uart_set_mode(MB_UART_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));

    ESP_LOGI(TAG, "Driver RS485 inicializado com sucesso de forma nativa!");

    uint8_t buffer_resposta[128];

    while (1) {
        ESP_LOGI(TAG, "Solicitando dado ao PC...");
        enviar_requisicao_modbus_read_reg0();

        // Aguarda a resposta do PC no barramento RS485 por até 500ms
        int tamanho_resposta = uart_read_bytes(MB_UART_PORT_NUM, buffer_resposta, sizeof(buffer_resposta), pdMS_TO_TICKS(500));

        if (tamanho_resposta >= 5) {
            // Em uma resposta Modbus válida de leitura (Função 03), o valor do dado de 16 bits fica nos bytes 3 e 4
            uint16_t valor_registrador = (buffer_resposta[3] << 8) | buffer_resposta[4];
            ESP_LOGI(TAG, "Valor recebido do PC: %d", valor_registrador);
        } else if (tamanho_resposta == 0) {
            ESP_LOGE(TAG, "Timeout: O PC nao respondeu no barramento RS485.");
        } else {
            ESP_LOGW(TAG, "Dados incorretos ou incompletos recebidos.");
        }

        // Aguarda 1 segundo antes da próxima consulta
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
