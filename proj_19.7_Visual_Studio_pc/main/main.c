#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "driver/gpio.h"

#define WIFI_SSID      "HOME PONTE"
#define WIFI_PASS      "GRA13DAN28"
#define PORT           80
#define BLINK_GPIO     48 // Ajuste para o pino do LED da sua placa ESP32-S3

static const char *TAG = "ESP32_S3_TCP";

// Manipulador de eventos de Wi-Fi e IP
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Tentando reconectar ao Wi-Fi...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado! IP do ESP32-S3: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Inicialização do Wi-Fi no modo Station (STA)
void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Cria a estrutura padrão de rede
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    // --- BLOCO DE CONFIGURAÇÃO DE IP ESTÁTICO ---
    // PARA O LOG: Para o roteador liberar o tráfego, o IP deve estar na mesma faixa da sua rede.
    // Se o IP do seu computador começar com 192.168.0.X, mude os valores abaixo para 0 em vez de 1.
    esp_netif_dhcpc_stop(sta_netif); // Desativa o DHCP automático que está falhando

    esp_netif_ip_info_t ip_info;
   ip_info.ip.addr = esp_ip4addr_aton("192.168.15.250");       // O IP fixo do seu ESP32-S3
   ip_info.gw.addr = esp_ip4addr_aton("192.168.15.1");         // CORRIGIDO: O IP do seu Roteador (Gateway)
   ip_info.netmask.addr = esp_ip4addr_aton("255.255.255.0");  // Máscara padrão

    esp_netif_set_ip_info(sta_netif, &ip_info);
    // --------------------------------------------

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


// Task que gerencia o Servidor TCP
static void tcp_server_task(void *pvParameters) {
    char rx_buffer[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Não foi possível criar o socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        ESP_LOGE(TAG, "Erro no bind do socket: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_sock, 1) != 0) {
        ESP_LOGE(TAG, "Erro durante o listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        ESP_LOGI(TAG, "Aguardando conexão do Visual Studio...");
        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Não foi possível aceitar a conexão: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Visual Studio conectado!");

        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Erro ao receber dados: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGI(TAG, "Visual Studio fechou a conexão");
                break;
            } else {
                rx_buffer[len] = 0; // Garante o caractere nulo de fim de string
                ESP_LOGI(TAG, "Recebido %d bytes: %s", len, rx_buffer);

                if (rx_buffer[0] == 'H') {
                    gpio_set_level(BLINK_GPIO, 1);
                    send(sock, "LED Aceso\n", 10, 0);
                } else if (rx_buffer[0] == 'L') {
                    gpio_set_level(BLINK_GPIO, 0);
                    send(sock, "LED Apagado\n", 12, 0);
                }
            }
        }

        shutdown(sock, 0);
        close(sock);
    }
    close(listen_sock);
    vTaskDelete(NULL);
}

void app_main(void) {
    // Inicializa a NVS (obrigatório para o Wi-Fi funcionar no IDF)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa o pino do LED
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // Inicializa Wi-Fi e cria a Task TCP
    wifi_init_sta();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
