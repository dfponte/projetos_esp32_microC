
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "string.h"
#include "esp_log.h"

static const char *TAG = "UART Basic";

//UART1 pins
#define TXD1_PIN 10         // TXD1 is GPIO10
#define RXD1_PIN 11         // RXD1 is GPIO11

#define RX_BUF_SIZE 1024    // UART buffer size     

void app_main(void)
{
  // Configure UART0 parameters  
  uart_config_t uart_config = {
      .baud_rate = 9600,                            // Baudrate
      .data_bits = UART_DATA_8_BITS,                // Data bits
      .parity = UART_PARITY_DISABLE,                // Disable parity
      .stop_bits = UART_STOP_BITS_1,                // Number of stop bits
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,        // Disable flow control
      .source_clk = UART_SCLK_DEFAULT,              // Default clock
    };
  uart_param_config(UART_NUM_0, &uart_config);                        // Configure UART0 parameters
  uart_driver_install(UART_NUM_0, RX_BUF_SIZE, 0, 0, NULL, 0);        // Install UART driver for interrupt-driven reads and writes  

    // Configure UART1 parameters
  uart_config_t uart_config1 = {
      .baud_rate = 9600,                      // Baudrate
      .data_bits = UART_DATA_8_BITS,            // Data bits
      .parity = UART_PARITY_DISABLE,            // Disable parity
      .stop_bits = UART_STOP_BITS_1,            // Number of stop bits
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE     // Disable flow control
    };
  uart_param_config(UART_NUM_1, &uart_config1);                                         // Configure UART1 parameters
  uart_set_pin(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE); // Set UART1 pins(TX: IO10, RX: I011)
  uart_driver_install(UART_NUM_1, RX_BUF_SIZE, 0, 0, NULL, 0);                          // Install UART driver for interrupt-driven reads and writes


  // Send data to UART0  
  char message[] = "Hello From ESP32-S2\r\n";                // Message to send

  for(int i = 0; i < 5; i++)                                  // Loop to send the message
  {
    ESP_LOGI(TAG, "sending: %s", message);                    // Print the message to the console
    uart_write_bytes(UART_NUM_1, message, sizeof(message));   // Send the message to UART0
    vTaskDelay(pdMS_TO_TICKS(1000));                            // Delay 1 second
  }
  
  uint8_t *data = (uint8_t *) malloc(RX_BUF_SIZE);         // Allocate memory to store the data received
  
  while (1)
  {
    int len = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, pdMS_TO_TICKS(20));  // Read data from UART0
  
    if(len)   
    {
      data[len] = 0;                                    // Add the null character to the end of the string
      ESP_LOGI(TAG, "received: %s", data);              // Print the message to the console
      //uart_write_bytes(UART_NUM_1, data, sizeof(data)); // Send the message to UART1
    }
  }
}