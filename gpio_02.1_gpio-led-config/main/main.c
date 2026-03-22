/**
 * @file main.c
 * 
 * @author daniel f ponte (danyelponte@gmail.com)
 * @brief The project turn on/off 3 leds using interruption buttons;
 * @version 0.1
 * @date 2025-12-16
 * 
 * @copyright Copyright (c) 2025
 * 
 *************************************************************************/
#include <stdio.h>

//FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

//ESP-IDF includes
#include "driver/gpio.h"
#include "esp_log.h"
/**
 * @brief LEDS output and button input
 */
#define LED1_PIN 14   
#define LED2_PIN 21
#define LED3_PIN 33
#define BUTTON1_PIN 4
#define BUTTON2_PIN 15 

static const char* TAG = "BUTTON_LED_CTRL";                 //tag for ESP_LOGx

//queue handle
static QueueHandle_t gpio_evt_queue = NULL;             //queue to handle gpio events

//ISR handler
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;                     //get the GPIO number
    xQueueSendFromISR(gpio_evt_queue , &gpio_num, NULL);    //send the GPIO number that generated the interrupt to the queu
}


//button task    
void buttonTask(void *pvpameters)
{
    uint32_t gpio_num;                  //gpio number variable
    uint8_t led1 = 0, led2 = 0;         //led state variables
    TickType_t last_button_press = 0;   //last button press time variable

    while (true) 
    {
        xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY);        //wait for gpio event from ISR
        ESP_LOGI(TAG, "GPIO[%li] intr \n", gpio_num);                   //LOG the GPIO number       
        
        TickType_t current_time = xTaskGetTickCount();                  //get current time

        if (current_time - last_button_press >= pdMS_TO_TICKS(250))     //check if 250ms has passed since last button press
        {
            last_button_press = current_time;                           //update last button press time

            if (gpio_num == BUTTON1_PIN)                                //check if button1 was pressed
            {
                gpio_set_level(LED1_PIN, led1 ^= 1);                    //toggle led1
            }
            else if (gpio_num == BUTTON2_PIN)                           //check if button2 was pressed
            {
                gpio_set_level(LED2_PIN, led2 ^= 1);                    //toggle led2
            }
        }
    }
}


void app_main(void)
{
       gpio_config_t gpio_io_conf;
      
       gpio_io_conf.pin_bit_mask = (1ULL<<LED1_PIN) | (1ULL<<LED2_PIN) | (1ULL<<LED3_PIN);
       gpio_io_conf.mode = GPIO_MODE_OUTPUT;
       gpio_io_conf.pull_up_en=GPIO_PULLUP_DISABLE;
       gpio_io_conf.pull_down_en=GPIO_PULLDOWN_DISABLE;
       gpio_io_conf.intr_type=GPIO_INTR_DISABLE;
       
       gpio_config(&gpio_io_conf);

       gpio_set_level(LED1_PIN,0);
       gpio_set_level(LED2_PIN,0);
       gpio_set_level(LED3_PIN,0);

       gpio_io_conf.pin_bit_mask = (1ULL<<BUTTON1_PIN) | (1ULL<<BUTTON2_PIN);
       gpio_io_conf.mode = GPIO_MODE_INPUT;
       gpio_io_conf.pull_up_en=GPIO_PULLUP_ENABLE;
       gpio_io_conf.pull_down_en=GPIO_PULLDOWN_DISABLE;
       gpio_io_conf.intr_type=GPIO_INTR_NEGEDGE;

       gpio_config(&gpio_io_conf);

       //-------------Interrupt Configuration ------------------------------
       
         gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));             //create queue to handle gpio events
         xTaskCreate(buttonTask, "buttonTask", 2048, NULL, 2, NULL);     //create button task

          //install gpio isr service and add isr handler for button1 and button2
         gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);                         //install gpio isr service
         gpio_isr_handler_add(BUTTON1_PIN, gpio_isr_handler, (void*) BUTTON1_PIN);  //add isr handler for button1
         gpio_isr_handler_add(BUTTON2_PIN, gpio_isr_handler, (void*) BUTTON2_PIN); //add isr handler for button2

       //-------------------------------------------------------------------
       while(true){
               
         //toogle led3
        gpio_set_level(LED3_PIN, 1);                //turn on led3
        vTaskDelay(pdMS_TO_TICKS(500));             //delay 500ms
        gpio_set_level(LED3_PIN, 0);                //turn off led3
        vTaskDelay(pdMS_TO_TICKS(500));             //delay 500ms
       }
       
}