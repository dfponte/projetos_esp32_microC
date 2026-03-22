
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_PIN_1 2

void app_main(void)
{
  
   gpio_reset_pin(LED_PIN_1);
   gpio_set_direction(LED_PIN_1,GPIO_MODE_DEF_OUTPUT);

   while(1){
     gpio_set_level(LED_PIN_1,0);
     printf("LED ON \n");
     vTaskDelay(1000/portTICK_PERIOD_MS);
     gpio_set_level(LED_PIN_1,1);
     printf("LED OFF \n");
     vTaskDelay(1000/portTICK_PERIOD_MS);
   }
  
}


