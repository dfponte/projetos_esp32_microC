#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define PIN_LED_21   21
#define PIN_BUTTON_4  4 

static int valor =  0;
static const char* TAG = "ESP-INTER";

//Rotina de Interrupção
void isr_button_int(void* arg){
      valor++;
      static bool estado = 0;
      estado = (estado==1) ? 0:1;
      gpio_set_level(PIN_LED_21,estado);
      gpio_intr_disable(PIN_BUTTON_4);
}


void app_main(void)
{
    //Conf. Pino 21
     gpio_reset_pin(PIN_LED_21);
     gpio_set_direction(PIN_LED_21,GPIO_MODE_OUTPUT);
     gpio_set_level(PIN_LED_21,0);

    //Conf. Pino 4
    gpio_reset_pin(PIN_BUTTON_4);
    gpio_set_direction(PIN_BUTTON_4,GPIO_MODE_INPUT);
    gpio_pullup_en(PIN_BUTTON_4);

    //Conf. Interrupção
    gpio_set_intr_type(PIN_BUTTON_4,GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BUTTON_4,isr_button_int,(void*)PIN_BUTTON_4);

    while(true){
       ESP_LOGI(TAG,"Habilitando Interrupção");
       gpio_intr_enable(PIN_BUTTON_4);
       printf("Aguardando interrupção.......\n");
       vTaskDelay(1000/portTICK_PERIOD_MS);
       printf("Valor contado %d\n",valor);
       if(valor==10){
        valor = 0;
       }
    }

}
