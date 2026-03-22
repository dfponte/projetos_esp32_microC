#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "relay.h"




//Define os pinos do relé
#define RELAY1_PIN 21  //GPIO21
#define RELAY2_PIN 33  //GPIO33



void app_main(void)
{
    Relay relay1,relay2;

    relay_init(&relay1,RELAY1_PIN);
    relay_init(&relay2,RELAY2_PIN);   

    while (true)
    {
        relay_set_on(&relay1);
        printf("Status do relay1:%d\n",relay_get_status(&relay1));
        vTaskDelay(1000/portTICK_PERIOD_MS);
        relay_set_on(&relay2);
         printf("Status do relay2:%d\n",relay_get_status(&relay2));
        vTaskDelay(1000/portTICK_PERIOD_MS);
        relay_set_off(&relay1);
         printf("Status do relay1:%d\n",relay_get_status(&relay1));
        vTaskDelay(1000/portTICK_PERIOD_MS);
        relay_set_off(&relay2);
        printf("Status do relay2:%d\n",relay_get_status(&relay2));
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
   
  
}