#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LED_PIN_2 2
#define BUTTON_PIN_21 21

static const char *TAG = "BUTTON TESTE";

void app_main(void)
{  
   int btn_state = 1;
   bool i = 0;
   gpio_reset_pin(LED_PIN_2);
   gpio_set_direction(LED_PIN_2,GPIO_MODE_OUTPUT);

   gpio_reset_pin(BUTTON_PIN_21);
   gpio_set_direction(BUTTON_PIN_21,GPIO_MODE_INPUT);
   gpio_set_pull_mode(BUTTON_PIN_21,GPIO_PULLUP_ONLY);

   ESP_LOGI(TAG,"INIT");

   while(1){
      
     int new_state = gpio_get_level(BUTTON_PIN_21);

     if(new_state != btn_state){
         btn_state = new_state;
         if(btn_state==0){
              gpio_set_level(LED_PIN_2,i^=1);
              ESP_LOGI(TAG,"BUTTON PRESSED");
         }else{
               ESP_LOGI(TAG,"BUTTON RELEASED");
         }

     }
     vTaskDelay(1000/portTICK_PERIOD_MS);
   }
}