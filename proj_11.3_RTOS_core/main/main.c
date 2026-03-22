#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED1_PIN 14
#define LED2_PIN 21

static const char* TAG1 = "TASK MAIN!!";
static const char* TAG2 = "TASK BLINK!!";
static const char* TAG3 = "TASK  COUNTER!!";

void vTaskBlink(void *pvParameters);
void vTaskCounter(void *pvParameters);

TaskHandle_t xTaskBlink1Handle = NULL;
TaskHandle_t xTaskBlink2Handle = NULL;
TaskHandle_t xTaskCounterHandle = NULL;

#define BLINK1_PERIOD 250
#define BLINK2_PERIOD 500

typedef struct{
  int led_pin;
  int blink_period;
}Led_t;

Led_t Led1 = {LED1_PIN,BLINK1_PERIOD};
Led_t Led2 = {LED2_PIN,BLINK2_PERIOD};

int valor = 150;

void app_main(void)
    {
   ESP_LOGI(TAG1,"Inicinado o programa ...");
   
   xTaskCreatePinnedToCore(   //Criando Tarefa para core 
    vTaskBlink,
    "TaskBlink1",
    2048,
    (void*)&Led1,
    1,
    &xTaskBlink1Handle,
    0                       // Core 1
   );

   xTaskCreatePinnedToCore(
    vTaskBlink,
    "TaskBlink2",
    2048,
    (void*)&Led2,
    1,
    &xTaskBlink2Handle,
    0
   );

    xTaskCreatePinnedToCore(
     vTaskCounter,
     "TaskCounter",
     2048,
     (void*)valor,
     2,
     &xTaskCounterHandle,
     1
   );
    
   int i = 0;
   while (true)
   {
    //ESP_LOGI(TAG1,"Task main() - Counter:%d",i++); //Contador main
    //Avaliando a ocupação da memória com as tasks
    /**
     * @brief 
     * Ela retorna a menor quantidade de espaço livre (o "ponto mais baixo" de folga) que a pilha de uma tarefa teve 
     * desde que foi criada. Se o valor retornado for próximo de zero, a tarefa quase estourou a memória alocada e você precisa aumentá-la.
     * Se o valor for muito alto, você está desperdiçando RAM e pode reduzir o tamanho da pilha na função xTaskCreate. 
     * 
     */
    //ATENÇÃO ESP_LOGD NÃO VEM HABILITADO POR PADRÃO
    
    ESP_LOGD(TAG1,"Iniciando a [%s] : core [%d] ",pcTaskGetName(NULL),xPortGetCoreID());
    ESP_LOGI(TAG1,"Task Blink1 High Water Mark %d",uxTaskGetStackHighWaterMark(xTaskBlink1Handle));
    ESP_LOGI(TAG1,"Task Blink2 High Water Mark %d",uxTaskGetStackHighWaterMark(xTaskBlink2Handle));
    ESP_LOGI(TAG1,"Task Counter High Water Mark %d",uxTaskGetStackHighWaterMark(xTaskCounterHandle));
    vTaskDelay(pdMS_TO_TICKS(1000));
   }
   
}
    
void vTaskBlink(void *pvParameters){

    int led_pin = ((Led_t*)pvParameters)->led_pin;
    int blink_period = ((Led_t*)pvParameters)->blink_period;
    bool status = false;
    

    //ATENÇÃO LOGD NÃO VEM HABILITADO POR PADRÃO
    ESP_LOGD(TAG2,"Iniciando a [%s] : core [%d] ",pcTaskGetName(NULL),xPortGetCoreID());      // log message
    gpio_set_direction(led_pin,GPIO_MODE_OUTPUT); // set the pin output
    
    while (true)
    {
            /* code */
        gpio_set_level(led_pin,status^=1);
        ESP_LOGI(TAG2,"[%s] LED[%d]:%d",pcTaskGetName(NULL),led_pin,status);
        vTaskDelay(pdMS_TO_TICKS(blink_period));
    }
    

}

void vTaskCounter(void *pvParameters){
    int i = (int)pvParameters;

    //ATENÇÃO LOGD NÃO VEM HABILITADO POR PADRÃO

    ESP_LOGD(TAG3,"Iniciando a [%s] : core [%d] ",pcTaskGetName(NULL),xPortGetCoreID());
    while (true)
    {
        ESP_LOGI(TAG3,"Task Counter :%d",i++);
        vTaskDelay(pdMS_TO_TICKS(1000));

    }

}