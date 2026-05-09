#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GPS_FIX";

// Pinos testados e seguros para o ESP32-S3 (Evitam LED e Flash)
#define UART_PORT_NUM      UART_NUM_1
#define TXD_PIN            17 
#define RXD_PIN            16 
#define BUF_SIZE           1024

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema...");

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 1. Instalação limpa (sem deletar antes para evitar o erro NULL)
    if (!uart_is_driver_installed(UART_PORT_NUM)) {
        ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    }
    
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART OK. Conecte o TX do GPS no GPIO 16.");

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Leitura com timeout curto
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        
        if (len > 0) {
            data[len] = '\0';
            // Usa printf direto para evitar overhead de log em strings longas
            printf("%s", (char *)data);
        }
        
        // Pequeno delay para o Watchdog não resetar o chip
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
