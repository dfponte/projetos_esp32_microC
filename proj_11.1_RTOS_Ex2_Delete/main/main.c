#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN 14
static const char* TAG1 = "TASK MAIN!!";
static const char* TAG2 = "TASK BLINK!!";
static const char* TAG3 = "TASK  COUNTER!!";

void vTaskBlink(void *pvParameters);
void vTaskCounter(void *pvParameters);

TaskHandle_t xTaskBlinkHandle = NULL;
TaskHandle_t xTaskCounterHandle = NULL;

void app_main(void)
{
   ESP_LOGI(TAG1,"Inicinado o programa ...");

   xTaskCreate(
    vTaskBlink,
    "TaskBlink",
    2048,
    NULL,
    1,
    &xTaskBlinkHandle
   );

    xTaskCreate(
     vTaskCounter,
     "TaskCounter",
     2048,
     NULL,
     2,
     &xTaskCounterHandle
   );
    
   int i = 0;
   while (true)
   {
    ESP_LOGI(TAG1,"Task main() - Counter:%d",i++);
    vTaskDelay(pdMS_TO_TICKS(1000));
   }
   
}

void vTaskBlink(void *pvParameters){
    bool status = false;
    ESP_LOGI(TAG2,"Iniciando a task Blink");       // log message
    gpio_set_direction(LED_PIN,GPIO_MODE_OUTPUT); // set the pin output
    
    while (true)
    {
        /* code */
        gpio_set_level(LED_PIN,status^=1);
        ESP_LOGI(TAG2,"Task blink LED[%d]:%d",LED_PIN,status);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    

}

void vTaskCounter(void *pvParameters){
    int i = 1010;
     ESP_LOGI(TAG3,"Iniciando a task Counter");
    while (true)
    {
        if(i==1020){             //Evite o core panico chamando novamente a task
          vTaskDelete(xTaskBlinkHandle);
          xTaskBlinkHandle = NULL;  //Desnecessário
           gpio_set_level(LED_PIN,0);
        }else if(i==1025){
        // vTaskDelete(NULL);//Não precisa passar xTaskCounterHandle
        
        xTaskCreate(    // Criar novamente a task blink
        vTaskBlink,
        "TaskBlink",
        2048,
        NULL,
        1,
        &xTaskBlinkHandle
        );
          vTaskDelete(NULL);
        }



        ESP_LOGI(TAG3,"Task Counter :%d",i++);
        vTaskDelay(pdMS_TO_TICKS(1000));

    }

}