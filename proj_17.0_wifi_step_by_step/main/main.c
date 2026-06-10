#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

// Tag que aparecerá no terminal identificando as mensagens de aula
static const char *ALULA_TAG = "CURSO_ESP32S3";

// Definições fictícias para o teste de conexão do aluno
#define WIFI_SSID      "GalaxyDp"
#define WIFI_PASS      "Daniel13"

// Função que trata os eventos disparados pelo Wi-Fi e pelo lwIP
static void manipulador_de_eventos(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGW(ALULA_TAG, "[EVENTO] Wi-Fi Ligou! Tentando conectar ao SSID: %s...", WIFI_SSID);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE(ALULA_TAG, "[EVENTO] Falha na conexão ou desconectado. Rádio tentando reconectar...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGW(ALULA_TAG, "[EVENTO SUCCESS] O lwIP (DHCP) atribuiu o IP: " IPSTR " ao ESP32-S3!", IP2STR(&event->ip_info.ip));
    }
}

void app_main(void) 
{
    ESP_LOGW(ALULA_TAG, "=================================================================");
    ESP_LOGI(ALULA_TAG, "INICIANDO AULA PRÁTICA: FLUXO DE INICIALIZAÇÃO DO WI-FI NO ESP32-S3");
    ESP_LOGW(ALULA_TAG, "=================================================================");
    vTaskDelay(pdMS_TO_TICKS(3000)); // Pausa didática

    // -------------------------------------------------------------------------
    // PASSO 1: Memória Não Volátil (NVS Flash)
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 1] Inicializando a memória NVS Flash...");
    ESP_LOGD(ALULA_TAG, "-> Por que? O driver Wi-Fi precisa salvar dados de calibração do rádio PHY e credenciais.");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(ALULA_TAG, "-> NVS sem espaço ou versão antiga detectada. Limpando e reiniciando a partição...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGD(ALULA_TAG, "[OK] NVS Flash pronta.");
    vTaskDelay(pdMS_TO_TICKS(3000)); // Pausa de 3 segundos para o aluno ler

    // -------------------------------------------------------------------------
    // PASSO 2: Camada de Abstração de Rede (esp_netif)
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 2] Inicializando a esp_netif_init()...");
    ESP_LOGD(ALULA_TAG, "-> Por que? Cria a tarefa interna (thread) que gerencia a pilha TCP/IP do lwIP por baixo dos panos.");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGD(ALULA_TAG, "[OK] Pilha de rede lwIP alocada e rodando no background.");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // -------------------------------------------------------------------------
    // PASSO 3: Loop de Eventos do Sistema (Event Loop)
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 3] Criando o Loop de Eventos Padrão do Sistema...");
    ESP_LOGD(ALULA_TAG, "-> Por que? O ESP-IDF é assíncrono. Precisamos de um loop para receber alertas como 'Wi-Fi conectou' ou 'IP recebido'.");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGD(ALULA_TAG, "[OK] Loop de eventos padrão ativo.");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // -------------------------------------------------------------------------
    // PASSO 4: Criar Interface Lógica (Vincular Wi-Fi ao lwIP)
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 4] Amarrando o Driver de Hardware à Interface de Rede...");
    ESP_LOGD(ALULA_TAG, "-> Por que? A esp_netif_create_default_wifi_sta() cria o mapeamento lógico para o modo Estação (STA).");
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    ESP_LOGD(ALULA_TAG, "[OK] Vínculo lógico STA criado com sucesso.");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // -------------------------------------------------------------------------
    // PASSO 5: Configuração e Alocação do Hardware Wi-Fi
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 5] Alocando recursos de hardware do Wi-Fi...");
    ESP_LOGD(ALULA_TAG, "-> Por que? O esp_wifi_init() busca as configurações de clock, buffers e potência do rádio.");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGD(ALULA_TAG, "[OK] Estruturas de hardware do chip alocadas em memória.");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // -------------------------------------------------------------------------
    // PASSO 6: Registrar o Manipulador de Eventos (Event Handler)
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 6] Registrando funções de callback no Loop de Eventos...");
    ESP_LOGD(ALULA_TAG, "-> Por que? Indica ao sistema qual função em C deve ser executada quando o rádio emitir um sinal.");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &manipulador_de_eventos, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &manipulador_de_eventos, NULL, NULL));
    ESP_LOGD(ALULA_TAG, "[OK] Callbacks de Wi-Fi e IP registrados.");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // -------------------------------------------------------------------------
    // PASSO 7: Definir Modo de Operação e Passar Credenciais
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 7] Configurando Modo Estação (STA) e injetando as credenciais...");
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Garante segurança mínima WPA2
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGD(ALULA_TAG, "[OK] Modo STA definido. Credenciais salvas no registrador.");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // -------------------------------------------------------------------------
    // PASSO 8: Ligar o Rádio (Start)
    // -------------------------------------------------------------------------
    ESP_LOGI(ALULA_TAG, "[PASSO 8] Ligando fisicamente o rádio Wi-Fi (esp_wifi_start)...");
    ESP_LOGD(ALULA_TAG, "-> O que acontece agora? O rádio liga, dispara o evento WIFI_EVENT_STA_START e a nossa função callback assume.");
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGW(ALULA_TAG, "=================================================================");
    ESP_LOGW(ALULA_TAG, "FIM DO FLUXO DE INICIALIZAÇÃO. O ESP32-S3 AGORA RESPONDE VIA EVENTOS.");
    ESP_LOGW(ALULA_TAG, "=================================================================");

    // Mantém a tarefa principal viva apenas printando o status
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(ALULA_TAG, "[AULA] Monitorando... O loop principal está livre enquanto o Wi-Fi roda em paralelo.");
    }
}
