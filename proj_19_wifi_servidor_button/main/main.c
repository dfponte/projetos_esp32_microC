// freertos includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// esp-idf includes
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// websocket client includes
#include "esp_websocket_client.h"

// Configurações de Rede e Servidor (Altere com seus dados)
#define WIFI_SSID       "GalaxyDp"//"HOME PONTE"
#define WIFI_PASS       "Daniel13"
#define WS_SERVER_URI   "ws://10.78.168.215:3000" // Use o IP do seu PC/Servidor

// Configurações de Hardware
#define BUTTON_GPIO     GPIO_NUM_21   // Botão BOOT nativo da placa
#define LED_GPIO        GPIO_NUM_18  // Mude para o pino do seu LED se necessário

#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "ESP32_WS_FINAL";
static esp_websocket_client_handle_t ws_client = NULL;

// Handler de eventos do Wi-Fi STA
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGW(TAG, "Wi-Fi desconectado. Tentando reconectar...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado com Sucesso! IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Handler de eventos do WebSocket (Recebe comandos para o LED)
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado ao WebSocket do servidor com sucesso!");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Desconectado do servidor WebSocket.");
            break;
        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 0x01) { // Processa mensagens de texto recebidas
                char *mensagem = malloc(data->data_len + 1);
                if (mensagem == NULL) return;
                
                memcpy(mensagem, data->data_ptr, data->data_len);
                mensagem[data->data_len] = '\0'; // Garante o terminador de string nulo

                ESP_LOGI(TAG, "[WS Recebido] Comando LED: %s", mensagem);

                // Lógica de acionamento do LED baseado no comando do Node.js
                if (strcmp(mensagem, "ligar") == 0) {
                    gpio_set_level(LED_GPIO, 1);
                } else if (strcmp(mensagem, "desligar") == 0) {
                    gpio_set_level(LED_GPIO, 0);
                }
                free(mensagem);
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "Erro interno detectado no WebSocket.");
            break;
    }
}

// Inicializador da Pilha Wi-Fi
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// Tarefa 1: Monitoramento do Botão com Envio via WebSocket
void button_task(void *pvParameters)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    int ultimo_estado = 1; // Pull-up começa em nível Alto (1)

    while (1) {
        int estado_atual = gpio_get_level(BUTTON_GPIO);

        if (estado_atual != ultimo_estado) {
            vTaskDelay(pdMS_TO_TICKS(50)); // Debounce por Software
            if (gpio_get_level(BUTTON_GPIO) == estado_atual) {
                ultimo_estado = estado_atual;

                // Só transmite se o WebSocket estiver autenticado e online
                if (ws_client != NULL && esp_websocket_client_is_connected(ws_client)) {
                    if (estado_atual == 0) {
                        const char *msg = "botao:ligado";
                        esp_websocket_client_send_text(ws_client, msg, strlen(msg), portMAX_DELAY);
                        ESP_LOGI(TAG, "[WS Enviado] %s", msg);
                    } else {
                        const char *msg = "botao:desligar";
                        esp_websocket_client_send_text(ws_client, msg, strlen(msg), portMAX_DELAY);
                        ESP_LOGI(TAG, "[WS Enviado] %s", msg);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Tarefa 2: Conexão e manutenção do cliente WebSocket
void websocket_task(void *pvParameters)
{
    // Bloqueia e aguarda até que o Wi-Fi obtenha o endereço IP válido
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    esp_websocket_client_config_t websocket_cfg = {
        .uri = WS_SERVER_URI,
    };

    ESP_LOGI(TAG, "Iniciando cliente WebSocket apontando para %s...", WS_SERVER_URI);
    ws_client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)ws_client);
    esp_websocket_client_start(ws_client);

    vTaskDelete(NULL); // Deleta a própria task de configuração inicial
}

void app_main(void)
{
    // Configura o pino do LED digital como saída
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0); // Inicializa desligado

    // Inicialização segura da memória Flash NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Inicializando modo de operação ESP32-S3 STA");
    wifi_init_sta();

    // Criação das duas rotinas concorrentes no FreeRTOS
    xTaskCreate(&button_task, "button_task", 4096, NULL, 5, NULL);
    xTaskCreate(&websocket_task, "websocket_task", 4096, NULL, 5, NULL);
}
