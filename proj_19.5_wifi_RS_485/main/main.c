#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbcontroller.h"       

#define WIFI_SSID       "HOME PONTE"
#define WIFI_PASS       "GRA13DAN28"
#define SLAVE_ID        1       

#define MB_UART_PORT    UART_NUM_1
#define MB_TX_PIN       17
#define MB_RX_PIN       18
#define MB_RTS_PIN      16      

static const char *TAG = "TC900E_APP";

// Ponteiro global de contexto para o Modbus Master
static void* master_handler = NULL; 

typedef struct {
    float temperatura_s1;   
    float temperatura_s2;   
    uint16_t status_reles;  
} tc900e_data_t;

static tc900e_data_t dados_controlador;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Conexão perdida. Tentando reconectar ao Wi-Fi...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado com sucesso! IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static esp_err_t init_modbus_master(void) {
    mb_communication_info_t comm_info = {
        .baudrate = 9600, 
        .mode = MB_MODE_RTU,
        .port = MB_UART_PORT,
        .parity = MB_PARITY_NONE
    };
    
    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
    if (err != ESP_OK) return err;

    err = mbc_master_setup((void*)&comm_info);
    if (err != ESP_OK) return err;

    err = uart_set_pin(MB_UART_PORT, MB_TX_PIN, MB_RX_PIN, MB_RTS_PIN, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    err = uart_set_mode(MB_UART_PORT, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) return err;

    err = mbc_master_start();
    return err;
}

void modbus_reading_task(void *pvParameters) {
    esp_err_t err;
    uint16_t reg_buffer[3] = {0}; // Mudado para array para armazenar os 3 registradores lidos sequentialmente

    uint16_t target_register_start = 100; 
    uint8_t num_registers = 3;

    while (1) {
        // Inicialização limpa da estrutura aceita universalmente
        mb_param_request_t request;
        request.slave_addr = SLAVE_ID;
        request.command    = 3; // CORREÇÃO: Substituído pelo código de função Modbus "03" bruto (Read Holding Registers)
        request.reg_start  = target_register_start;
        request.reg_size   = num_registers;

        // Envia a requisição de leitura via RS485
        err = mbc_master_send_request(&request, (void*)reg_buffer);

        if (err == ESP_OK) {
            // Separa os valores lidos sequencialmente no array de buffer
            dados_controlador.temperatura_s1 = (int16_t)reg_buffer[0] / 10.0f;
            dados_controlador.temperatura_s2 = (int16_t)reg_buffer[1] / 10.0f;
            dados_controlador.status_reles   = reg_buffer[2];

            ESP_LOGI(TAG, "--- LEITURA REALIZADA COM SUCESSO ---");
            ESP_LOGI(TAG, "Temp S1 (Ambiente): %.1f CC", dados_controlador.temperatura_s1);
            ESP_LOGI(TAG, "Temp S2 (Evaporador): %.1f CC", dados_controlador.temperatura_s2);
            ESP_LOGI(TAG, "Status dos Reles: 0x%04X", dados_controlador.status_reles);
        } else {
            ESP_LOGE(TAG, "Erro na rede RS485 Modbus. Código: 0x%x", err);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); 
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    if (init_modbus_master() == ESP_OK) {
        ESP_LOGI(TAG, "Sistema Modbus Master iniciado!");
        xTaskCreate(modbus_reading_task, "modbus_task", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGE(TAG, "Erro fatal ao iniciar barramento serial.");
    }
}
