#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "calculadora.h"
#include "led.h"

void app_main(void)
{
    int res = somar(23,7);
    printf("Resultado da soma %d \n",res);
    liga_led();

    while(true){
       printf("Hello World\n");
       vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
}