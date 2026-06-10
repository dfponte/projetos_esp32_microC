#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <driver/gpio.h>
static const char *TAG = "UART_CONTROL";

#define TXD_PIN 4
#define RXD_PIN 5
#define UART_PORT_NUM UART_NUM_1
#define BUF_SIZE 1024

#define LED_PIN_21 21


void app_main(void) {
   
     char *reply = NULL;
     gpio_reset_pin(LED_PIN_21);
     gpio_set_direction(LED_PIN_21,GPIO_MODE_OUTPUT);
     gpio_set_level(LED_PIN_21,0);
     bool liga = 0;
     
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Inicialização da UART
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, -1, -1);
   
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    ESP_LOGI(TAG, "Sistema pronto. Envie '1' pelo adaptador serial.");

    while (1) {
        
        // Tenta ler dados da UART
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';
            
            // Verifica se o caractere recebido é '1'
            if (data[0] == '1') {
                // 1. Envia log para o seu monitor principal (USB do ESP32)
                ESP_LOGI(TAG, "COMANDO RECEBIDO: O valor '1' foi detectado!");
                 gpio_set_level(LED_PIN_21,liga^=1);
                 if(liga==1){
                       ESP_LOGI(TAG, "Ligado!");
                        reply = "Confirmado: Led Ligado\r\n";
                 }else{
                       ESP_LOGI(TAG, "Desligado!");
                        reply = "Confirmado: Led Desligado\r\n";
                 }
                // 2. Opcional: Envia uma resposta de volta para o adaptador
             // char *reply = "Confirmado: recebi info\r\n";
              
                uart_write_bytes(UART_PORT_NUM, reply, strlen(reply));
            } else {
                ESP_LOGW(TAG, "Recebido algo diferente: %s", (char *)data);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
