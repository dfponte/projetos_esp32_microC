#include <stdio.h>

//RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//ESP-IDF
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define LSB_SIZE_3V3         0.0008056f // Valor de 1 bit para escala 6.144V

static int adc_raw;     // ADC raw data
static const char* TAG = "ADC TEST";


//-------------------------------------------------
void app_main(void)
{
  adc_oneshot_unit_handle_t adc1_handler;
  adc_oneshot_unit_init_cfg_t init_config1 ={
    .unit_id = ADC_UNIT_1
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1,&adc1_handler));
  
 adc_oneshot_chan_cfg_t config = {
    .bitwidth = ADC_BITWIDTH_12,   //12 bits
    .atten = ADC_ATTEN_DB_12

 };
  
 ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handler,ADC_CHANNEL_0,&config));
   
 
 
 while(true){
      
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handler,ADC_CHANNEL_0,&adc_raw));
    
    ESP_LOGI(TAG,"ADC%d Channel[%d] Raw Data: %d",ADC_UNIT_1+1,ADC_CHANNEL_0,adc_raw);
    float voltage = (adc_raw)*(LSB_SIZE_3V3);
    printf("CH0: %d | %.3f V  \n",adc_raw, voltage);

    vTaskDelay(1000/portTICK_PERIOD_MS);
    // Bits 
               
 }

}