#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define TXD_PIN 43  // Pino padrão TX UART0 no S3
#define RXD_PIN 44  // Pino padrão RX UART0 no S3
#define UART_PORT UART_NUM_0
#define BUF_SIZE (1024)

void app_main(void) {
    // 1. Configuração da UART
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 2. Instalar o driver (Buffer RX é obrigatório para receber)
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Opcional: Loopback interno (Software)
    // Se descomentar a linha abaixo, você não precisa de fios externos
    // uart_set_loop_back(UART_PORT, true); 

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    const char* test_str = "TESTE_UART_S3";

    while (1) {
        // Envia o dado
        uart_write_bytes(UART_PORT, test_str, strlen(test_str));
        ESP_LOGI("UART", "Enviado: %s", test_str);

        // Tenta ler o dado (Timeout de 1 segundo)
        int len = uart_read_bytes(UART_PORT, data, BUF_SIZE - 1, 1000 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI("UART", "Recebido (%d bytes): %s", len, data);
        } else {
            ESP_LOGE("UART", "Erro: Nenhum dado recebido no RX!");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
