/**
 * @file main.c
 * @author Daniel F Ponte (you@domain.com)
 * @brief Criação de duas tarefas: uma envia informação para
 * fila e a outra consome inf. da fila.
 * @version 0.1
 * @date 2026-02-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define LED_14 14

static const char *TAG= "FILA:";

// Criação dos handles para as tasks
QueueHandle_t xQueue;

TaskHandle_t xTask1Handle;
TaskHandle_t xTask2Handle;

// Protótipos de função para as tasks

void vTask1(void *pvParameters);
void vTask2(void *pvParameters);


void app_main(void)
{

  xQueue = xQueueCreate(5,sizeof(int));
  
  xTaskCreate(vTask1,"Task1",configMINIMAL_STACK_SIZE+1024,NULL,1,&xTask1Handle);
  xTaskCreate(vTask2,"Task2",configMINIMAL_STACK_SIZE+1024,NULL,1,&xTask2Handle);

  gpio_set_direction(LED_14,GPIO_MODE_OUTPUT);
  bool status = false;
  while(true){
     gpio_set_level(LED_14,status^=1);
     vTaskDelay(pdMS_TO_TICKS(1000));
  }
  ESP_LOGI(TAG,"");

}

//Tarefa responsável por enviar inf. na fila
void vTask1(void *pvParameters){
    int count = 0;
    while(true){
       if(count<10){
         xQueueSend(xQueue,&count,portMAX_DELAY);
         count++;
       }else{
        count = 0;
        vTaskDelay(pdMS_TO_TICKS(500));
       }
       vTaskDelay(pdMS_TO_TICKS(500));
    }

}


void vTask2(void *pvParameters){
    int received_count = 0;
    while(true){
         // Tempo receber pdMS_TO_TICKS(1000) que pode gerera um TIMEOUT
         //Ou portMax_DELAY que espera receber algo...
         if(xQueueReceive(xQueue,&received_count,portMAX_DELAY)==pdTRUE){
          ESP_LOGI(TAG,"received: %d ",received_count);

         }else{
             ESP_LOGI(TAG,"TIMEOUT ");
         }
    }

}