#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN 14
static const char* TAG = "TASK ON!!";

TaskHandle_t xTaskBlinkHandle = NULL;

void vTaskBlink(void *pvParameters);

void app_main(void)
{
   ESP_LOGI(TAG,"Inicinado o programa ...");

   xTaskCreate(
    vTaskBlink,
    "TakBlink",
    2048,
    NULL,
    1,
    &xTaskBlinkHandle
   );
    
   int i = 0;
   while (true)
   {
    ESP_LOGI(TAG,"Task main() - Counter:%d",i);
    vTaskDelay(pdMS_TO_TICKS(1000));
   }
   
}

void vTaskBlink(void *pvParameters){
    bool status = false;
    ESP_LOGI(TAG,"Iniciando a task Blink");       // log message
    gpio_set_direction(LED_PIN,GPIO_MODE_OUTPUT); // set the pin output
    
    while (true)
    {
        /* code */
        gpio_set_level(LED_PIN,status^=1);
        ESP_LOGI(TAG,"Task blink LED[%d]:%d",LED_PIN,status);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    

}