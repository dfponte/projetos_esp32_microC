#include <stdio.h>
#include <string.h>
#include <wifi.h>


//freertos includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

//esp-idf includes
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

//http client includes
#include "esp_http_client.h"

//tag for logging
static const char *TAG = "WIFI Example";

//-----------------------------------------//

void app_main(void)
{

//-------Inicializa a NVS----------------------//

  //initialize NVS
    esp_err_t ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

//-------------Inicializa a estrutura pilha TCP/IP------------------------//

ESP_LOGI(TAG,"ESP_WIFI_MODE_AP");
//ESP_ERROR_CHECK(wifi_init_sta());
wifi_connect_ap("REDE TESTE","1234567890");

}