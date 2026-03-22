#include <stdio.h>
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#define LED1_PIN  21 //Led acende com a interrupção
#define LED2_PIN  22 //Led acende com a interrupção
#define LED3_PIN  33 //Led acende com a interrupção

#define BUTTON1_PIN 2  
#define BUTTON2_PIN 4  

static const char *TAG ="START_INT";

//queue handle


static QueueHandle_t gpio_evt_queue = NULL;                 //queue to handle gpio events


static void IRAM_ATTR gpio_isr_handler(void *arg){

  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue,&gpio_num,NULL);

}

void buttonTask(void *pvparameters){
    uint32_t gpio_num;
    while (true)
    {
        /* code */
        xQueueReceive(gpio_evt_queue,&gpio_num,portMAX_DELAY);
        ESP_LOGI(TAG, "GPIO[%li] intr \n", gpio_num);                   //LOG the GPIO number
        ESP_LOGI(TAG,"TESTE");
      }                        
}

void app_main(void)
{
  
  //Config. of Pins Out;
  gpio_reset_pin(LED1_PIN);
  gpio_set_direction(LED1_PIN,GPIO_MODE_OUTPUT);
  gpio_reset_pin(LED2_PIN);
  gpio_set_direction(LED2_PIN,GPIO_MODE_OUTPUT);
  gpio_reset_pin(LED3_PIN);
  gpio_set_direction(LED3_PIN,GPIO_MODE_OUTPUT);
  
  gpio_set_level(LED1_PIN,0);
  gpio_set_level(LED2_PIN,0);
  gpio_set_level(LED3_PIN,0);

  //Config. of BUTTON1_PIN as input;
  gpio_reset_pin(BUTTON1_PIN);
  gpio_set_direction(BUTTON1_PIN,GPIO_MODE_INPUT);
  gpio_pullup_en(BUTTON1_PIN);
  gpio_pulldown_dis(BUTTON1_PIN); 
  gpio_set_intr_type(BUTTON1_PIN,GPIO_INTR_NEGEDGE);

  gpio_set_direction(BUTTON2_PIN,GPIO_MODE_INPUT);
  gpio_pullup_en(BUTTON2_PIN);
  gpio_pulldown_dis(BUTTON2_PIN);                     //disable pull-down
  
  
 
  
  gpio_evt_queue = xQueueCreate(1,sizeof(uint32_t));
  xTaskCreate(buttonTask,"BUTTON_TASK",2048,NULL,2,NULL);
  
  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
  gpio_isr_handler_add(BUTTON1_PIN,gpio_isr_handler,(void*)BUTTON1_PIN);


    while (true)
  {
    gpio_set_level(LED3_PIN,0);
    vTaskDelay(1000/portTICK_PERIOD_MS);
     gpio_set_level(LED3_PIN,1);
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  
}