#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "Program Logging";


void app_main(void)
{

printf("Log level para Default == INFO \n");
ESP_LOGE(TAG,"Erro....");
ESP_LOGW(TAG,"Warning");
ESP_LOGI(TAG,"Informação");
ESP_LOGD(TAG,"Debug");
ESP_LOGV(TAG,"Verbose");

//Alterando default

printf("Log level para ERRO \n");
esp_log_level_set(TAG,ESP_LOG_ERROR);
ESP_LOGE(TAG,"Erro....");
ESP_LOGW(TAG,"Warning");
ESP_LOGI(TAG,"Informação");
ESP_LOGD(TAG,"Debug");
ESP_LOGV(TAG,"Verbose");

printf("Log level para INFO \n");
esp_log_level_set(TAG,ESP_LOG_INFO);
ESP_LOGE(TAG,"Erro....");
ESP_LOGW(TAG,"Warning");
ESP_LOGI(TAG,"Informação");
ESP_LOGD(TAG,"Debug");
ESP_LOGV(TAG,"Verbose");

printf("Log level para NONE \n");
esp_log_level_set(TAG,ESP_LOG_NONE);
ESP_LOGE(TAG,"Erro....");
ESP_LOGW(TAG,"Warning");
ESP_LOGI(TAG,"Informação");
ESP_LOGD(TAG,"Debug");
ESP_LOGV(TAG,"Verbose");

}