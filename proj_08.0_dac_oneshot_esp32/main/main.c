#include <stdio.h>

//FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/dac_oneshot.h"
#include "esp_log.h"

static const char* TAG = "DAC-oneshot";



void app_main(void)
{
    uint8_t val = 0;

    dac_oneshot_handle_t chan0_handle;              //DAC channel 0 handle
    dac_oneshot_config_t chan0_cfg = {              //DAC channel 0 configuration
        .chan_id = DAC_CHAN_0                       //DAC channel 0 
        };

    ESP_ERROR_CHECK(dac_oneshot_new_channel(&chan0_cfg,&chan0_handle));  // Create DAC channel 0
 
    ESP_LOGI(TAG,"DAC oneshot start");

    while(true){

       for(val=0;val<255;val++){
        ESP_ERROR_CHECK(dac_oneshot_output_voltage(chan0_handle,val));
        vTaskDelay(1/portTICK_PERIOD_MS);
       }
    
        for(val=255;val>0;val--){
        ESP_ERROR_CHECK(dac_oneshot_output_voltage(chan0_handle,val));
        vTaskDelay(1/portTICK_PERIOD_MS);
       }

       
    }

}