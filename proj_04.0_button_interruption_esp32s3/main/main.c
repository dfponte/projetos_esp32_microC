#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
//#include "esp_system.h"
//#include "esp_spi_flash.h"
#define LED1_21 21
#define BUTTON_4 4
static int valor  = 0;

static const char* TAG = "ESP-32S3-INTR";

//Rotina de Serviço de Interrupção
void isr_button_int(void* arg){
    valor++;
    static bool estado = 0;
    estado = (estado ==1) ? 0 : 1;
    gpio_set_level(LED1_21,estado);
    gpio_intr_disable(BUTTON_4);
   // while(!gpio_get_level(BUTTON_4));
}

void app_main(void)
{
    // Conf. Pino Led 21
    gpio_reset_pin(LED1_21);
    gpio_set_direction(LED1_21,GPIO_MODE_OUTPUT);
    gpio_set_level(LED1_21,0);

    // Conf. Pino Button pino 4
     gpio_reset_pin(BUTTON_4);
     gpio_set_direction(BUTTON_4,GPIO_MODE_INPUT);
     gpio_pullup_en(BUTTON_4);

     //Conf. da Interrupção

gpio_set_intr_type(BUTTON_4,GPIO_INTR_NEGEDGE);
gpio_install_isr_service(0);
gpio_isr_handler_add(BUTTON_4,isr_button_int,(void*)BUTTON_4);
gpio_intr_enable(BUTTON_4);

while(true){
    ESP_LOGI(TAG,"Interrupção Habilitada");
    gpio_intr_enable(BUTTON_4);
    printf("Aguardando interrupcao....\n");
    vTaskDelay(1000/portTICK_PERIOD_MS);
    printf(">>>>>>>>........Valor ?......<<<<<<<<\n");
    printf("Valor = %d\n",valor);
    if(valor==10){
         valor=0;2
    }
}

    }    
