#include<stdio.h>
//freeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//ESP32 includes
#include "driver/ledc.h"

//main task
void app_main(void)
{
    uint16_t duty = 0;    //duty cycle value

    //Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t timer_conf = {      //ledc timer struct
    .speed_mode = LEDC_LOW_SPEED_MODE,      //timer mode      
    .timer_num = LEDC_TIMER_0,              //timer number  
    .freq_hz = 5000,                        //frequency in Hz
    .duty_resolution = LEDC_TIMER_10_BIT,   //duty resolution
    .clk_cfg = LEDC_AUTO_CLK                //clock source
    };
    ledc_timer_config(&timer_conf);        //apply the configuration    

    //Prepare and then apply the LEDC PWM channel0 configuration
    ledc_channel_config_t channel_conf= {   //ledc channel struct
    .channel = LEDC_CHANNEL_0,              //channel number    
    .speed_mode = LEDC_LOW_SPEED_MODE,      //speed mode
    .timer_sel = LEDC_TIMER_0,              //select timer 
    .intr_type = LEDC_INTR_DISABLE,         //interrupt disabled
    .gpio_num = GPIO_NUM_14,                //GPIO number
    .duty = 0                              //duty cycle
    };
    ledc_channel_config(&channel_conf);     //apply the configuration

      //Prepare and then apply the LEDC PWM channel1 configuration
    ledc_channel_config_t channel_conf1= {   //ledc channel struct
    .channel = LEDC_CHANNEL_1,              //channel number    
    .speed_mode = LEDC_LOW_SPEED_MODE,      //speed mode
    .timer_sel = LEDC_TIMER_0,              //select timer 
    .intr_type = LEDC_INTR_DISABLE,         //interrupt disabled
    .gpio_num = GPIO_NUM_33,                //GPIO number
    .duty = 511                              //duty cycle
    };
    ledc_channel_config(&channel_conf1);     //apply the configuration


    ledc_fade_func_install(0);

    while(1) 
    {
         ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,512,5000,LEDC_FADE_WAIT_DONE);
         vTaskDelay(500/portTICK_PERIOD_MS);
         ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0,5000,LEDC_FADE_WAIT_DONE);
         vTaskDelay(500/portTICK_PERIOD_MS);
    }   
}