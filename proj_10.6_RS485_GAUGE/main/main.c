
/**
 Ele faz com que a placa se comporte exatamente como um
  dispositivo escravo em uma rede industrial, 
 respondendo a comandos de leitura e escrita de dados.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "TC900E_SIMULATOR";

// Configurações de Hardware
#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     9600       
#define TX_PIN             (17)       
#define RX_PIN             (16)       
#define RTS_PIN            (18)       
#define CTS_PIN            (UART_PIN_NO_CHANGE)

#define BUF_SIZE           (256)
#define TC900E_SLAVE_ID    (0x01)     

// --- VARIÁVEIS GLOBAIS DO SIMULADOR ---
// Agora os valores não são mais fixos. Você pode alterá-los em qualquer parte do código.
int16_t sensor_1_temp = 255;  // 25.5°C (Registrador 0)
int16_t sensor_2_temp = -50;  // -5.0°C (Registrador 1)
uint16_t setpoint     = 200;  // 20.0°C (Registrador 2 - Exemplo de Setpoint)

// Função de cálculo de CRC16 Modbus RTU
uint16_t modbus_crc(const uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void init_tc900e_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TX_PIN, RX_PIN, RTS_PIN, CTS_PIN));
    ESP_ERROR_CHECK(uart_set_mode(UART_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));
    ESP_LOGI(TAG, "Hardware RS485 inicializado.");
}

// Processa as requisições Modbus (Leitura e Escrita)
void process_modbus_request(uint8_t *request, int length) {
    if (length < 8) return;
    if (request[0] != TC900E_SLAVE_ID) return;

    uint16_t crc_recebido = (request[length - 1] << 8) | request[length - 2];
    if (modbus_crc(request, length - 2) != crc_recebido) {
        ESP_LOGW(TAG, "Erro de CRC.");
        return;
    }

    uint8_t function_code = request[1];
    uint8_t response[BUF_SIZE];
    int resp_len = 0;

    // --- TRATAMENTO DE LEITURA (Funções 03 ou 04) ---
    if (function_code == 0x03 || function_code == 0x04) { 
        uint16_t endereco_inicial = (request[2] << 8) | request[3];
        uint16_t qtd_registradores = (request[4] << 8) | request[5];

        uint8_t byte_count = qtd_registradores * 2;
        response[0] = TC900E_SLAVE_ID;
        response[1] = function_code;
        response[2] = byte_count;
        
        int index = 3;
        for (int i = 0; i < qtd_registradores; i++) {
            uint16_t reg_atual = endereco_inicial + i;

            if (reg_atual == 0) {
                response[index++] = (sensor_1_temp >> 8) & 0xFF;
                response[index++] = sensor_1_temp & 0xFF;
            } 
            else if (reg_atual == 1) {
                response[index++] = (sensor_2_temp >> 8) & 0xFF;
                response[index++] = sensor_2_temp & 0xFF;
            }
            else if (reg_atual == 2) {
                response[index++] = (setpoint >> 8) & 0xFF;
                response[index++] = setpoint & 0xFF;
            }
            else {
                response[index++] = 0x00;
                response[index++] = 0x00;
            }
        }

        uint16_t crc_resposta = modbus_crc(response, index);
        response[index++] = crc_resposta & 0xFF;
        response[index++] = (crc_resposta >> 8) & 0xFF;
        resp_len = index;
    } 
    // --- TRATAMENTO DE ESCRITA (Função 06 - Write Single Register) ---
    else if (function_code == 0x06) {
        uint16_t endereco_reg = (request[2] << 8) | request[3];
        int16_t valor_recebido = (request[4] << 8) | request[5];

        ESP_LOGI(TAG, "Escrita Modbus recebida. Reg: %d, Novo Valor: %d", endereco_reg, valor_recebido);

        // Aplica o novo valor dependendo do registrador enviado pelo mestre
        if (endereco_reg == 0) {
            sensor_1_temp = valor_recebido;
        } 
        else if (endereco_reg == 1) {
            sensor_2_temp = valor_recebido;
        }
        else if (endereco_reg == 2) {
            setpoint = valor_recebido;
        }

        // A resposta padrão da Função 06 é o eco exato do frame de requisição
        memcpy(response, request, length);
        resp_len = length;
    }
    else {
        // Exceção Modbus para funções inválidas
        response[0] = TC900E_SLAVE_ID;
        response[1] = function_code | 0x80;
        response[2] = 0x01; 
        uint16_t crc_resposta = modbus_crc(response, 3);
        response[3] = crc_resposta & 0xFF;
        response[4] = (crc_resposta >> 8) & 0xFF;
        resp_len = 5;
    }

    uart_write_bytes(UART_PORT_NUM, (const char *)response, resp_len);
}

// Uma tarefa secundária para simular variações de temperatura automaticamente
void temperatura_simulator_task(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // Aguarda 5 segundos
        
        // Simula a temperatura do sensor 1 oscilando levemente de 5 em 5 segundos
        sensor_1_temp += 1; 
        if (sensor_1_temp > 280) sensor_1_temp = 240; // Mantém entre 24.0°C e 28.0°C

        ESP_LOGD(TAG, "Simulação interna: Sensor 1 atualizado para %d", sensor_1_temp);
    }
}

void app_main(void) {
    init_tc900e_uart();

    // Cria a tarefa que altera os valores por código de forma independente
    xTaskCreate(temperatura_simulator_task, "temp_sim_task", 2048, NULL, 5, NULL);

    uint8_t *rx_data = (uint8_t *) malloc(BUF_SIZE);
    ESP_LOGI(TAG, "Simulador TC-900E ativo com suporte a Escrita.");

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, rx_data, BUF_SIZE, 50 / portTICK_PERIOD_MS);
        if (len > 0) {
            process_modbus_request(rx_data, len);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(rx_data);
}
