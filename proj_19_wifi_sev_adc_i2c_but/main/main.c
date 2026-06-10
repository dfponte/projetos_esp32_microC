#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"

// ==================== CONFIGURAÇÕES DE REDE ====================
#define WIFI_SSID        "GalaxyDp"   
#define WIFI_PASS        "Daniel13" 
#define SERVER_URL       "http://10.78.168.215" // URL completa com a rota /data

// ==================== CONFIGURAÇÕES DE HARDWARE ====================
#define PIN_BUTTON      GPIO_NUM_21   // Botão de Boot padrão do ESP32-S3
#define PIN_I2C_SDA     GPIO_NUM_4
#define PIN_I2C_SCL     GPIO_NUM_5
#define PIN_ADC_CH      ADC_CHANNEL_0 // GPIO 1 (Canal 0 do ADC1 no ESP32-S3)
#define BH1750_ADDR     0x23         // Endereço I2C padrão do sensor de luz

static const char *TAG = "ESP32_S3_APP";
static bool s_connected = false;

// Gerenciador de eventos do Wi-Fi e IP
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        ESP_LOGW(TAG, "Wi-Fi desconectado. Tentando reconectar...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado com sucesso! IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        s_connected = true;
    }
}

// Inicialização do Wi-Fi
static void wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi disparado, aguardando IP...");
}

// Inicialização dos Periféricos (ADC, I2C e GPIO)
static void peripherals_init(adc_oneshot_unit_handle_t *adc_handle) {
    // 1. Configuração do GPIO do Botão
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 2. Configuração do I2C para o Sensor de Luz
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &i2c_conf);
    i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0);

    vTaskDelay(pdMS_TO_TICKS(100)); // Pequena pausa para estabilização elétrica do barramento
    uint8_t cmd = 0x10; // Ativação contínua do BH1750
    i2c_master_write_to_device(I2C_NUM_0, BH1750_ADDR, &cmd, 1, pdMS_TO_TICKS(1000));

    // 3. Configuração do ADC OneShot Unit (Padrão ESP-IDF v5)
    adc_oneshot_unit_init_cfg_t init_config1 = { 
        .unit_id = ADC_UNIT_1,                 
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    adc_oneshot_new_unit(&init_config1, adc_handle);

    adc_oneshot_chan_cfg_t adc_config = { 
        .bitwidth = ADC_BITWIDTH_DEFAULT, 
        .atten = ADC_ATTEN_DB_12 
    };
    adc_oneshot_config_channel(*adc_handle, PIN_ADC_CH, &adc_config);
}

// Envio dos dados via HTTP POST
static void send_http_post(float luz, int adc, int botao) {
    if (!s_connected) {
        ESP_LOGW(TAG, "Envio cancelado: Placa sem conexão Wi-Fi ativa.");
        return;
    }

    char json_payload[256]; // Buffer expandido com margem de segurança contra estouro
    snprintf(json_payload, sizeof(json_payload), "{\"luz\":%.2f,\"adc\":%d,\"botao\":%d}", luz, adc, botao);

    esp_http_client_config_t config = {
        .url = SERVER_URL,                     
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_TCP, 
        .timeout_ms = 5000, // Evita travamento da Task se o servidor cair
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, (int)strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGI(TAG, "HTTP POST Sucesso: Status %d", status_code);
        } else {
            ESP_LOGW(TAG, "HTTP POST Erro de Rota no Servidor: Status %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "Falha Crítica na Conexão HTTP: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

void app_main(void) {
    adc_oneshot_unit_handle_t adc_handle;
    
    wifi_init();
    peripherals_init(&adc_handle);

    // Bloqueia a execução até que o Wi-Fi receba um IP válido pela primeira vez
    while (!s_connected) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    while (1) {
        // 1. Leitura do Botão (Invertido devido ao Pull-up)
        int botao_status = !gpio_get_level(PIN_BUTTON);

        // 2. Leitura do ADC
        int adc_raw = 0;
        adc_oneshot_read(adc_handle, PIN_ADC_CH, &adc_raw);

        // 3. Leitura do Sensor de Luz I2C
        uint8_t data[2] = {0};
        float lux = 0.0;
        if (i2c_master_read_from_device(I2C_NUM_0, BH1750_ADDR, data, 2, pdMS_TO_TICKS(100)) == ESP_OK) {
            lux = ((data[0] << 8) | data[1]) / 1.2;
        }

        ESP_LOGI(TAG, "Luz: %.2f Lx | ADC: %d | Botao: %d", lux, adc_raw, botao_status);

        // Dispara o envio HTTP
        send_http_post(lux, adc_raw, botao_status);

        // Intervalo de amostragem (2 segundos)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
