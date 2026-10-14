#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "ADS131M08";

// Definição dos pinos no ESP32-S3 (Utilizando SPI2_HOST)
#define PIN_NUM_MISO   13
#define PIN_NUM_MOSI   11
#define PIN_NUM_CLK    12
#define PIN_NUM_CS     10
#define PIN_NUM_DRDY   14
#define PIN_NUM_RESET  9

// Definições do protocolo ADS131M08
#define WORD_SIZE_BYTES   3    // 24 bits por palavra
#define NUMBER_OF_WORDS   10   // 1 Status + 8 Canais + 1 CRC
#define BUFFER_SIZE       (WORD_SIZE_BYTES * NUMBER_OF_WORDS) // 30 Bytes

static QueueHandle_t gpio_evt_queue = NULL;
static spi_device_handle_t spi_device;

// Manipulador de interrupção (ISR) para o pino DRDY
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Inicializa os pinos de controle GPIO
void init_gpio_control(void) {
    // Configura o pino de RESET
    gpio_config_t io_conf_reset = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_RESET),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf_reset);

    // Configura o pino DRDY (Interrupção na borda de descida)
    gpio_config_t io_conf_drdy = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_DRDY),
        .pull_down_en = 0,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf_drdy);

    // Cria a fila do FreeRTOS
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Instala o serviço de interrupção global do GPIO
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_NUM_DRDY, gpio_isr_handler, (void*) PIN_NUM_DRDY);

    // Pulsa o pino de Reset para inicializar o ADS131M08
    gpio_set_level(PIN_NUM_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_NUM_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// Inicializa o periférico SPI
void init_spi(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BUFFER_SIZE
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4 * 1000 * 1000, // 4 MHz
        .mode = 1,                         // SPI Modo 1 (CPOL=0, CPHA=1) para o ADS131
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
    };

    // Inicializa o barramento SPI2 (antigo HSPI)
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_device));
}

// Task que aguarda o DRDY e lê os dados do ADC
void ads131_task(void *pvParameters) {
    uint32_t io_num;
    uint8_t rx_buf[BUFFER_SIZE];
    uint8_t tx_buf[BUFFER_SIZE] = {0}; // Envia zeros para ler os dados (comando NULL)

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = BUFFER_SIZE * 8; // Comprimento em bits
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;

    ESP_LOGI(TAG, "Task do ADS131M08 iniciada.");

    while (1) {
        // Aguarda o pino DRDY sinalizar que os dados estão prontos
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            // Realiza a transferência SPI de forma síncrona
            esp_err_t ret = spi_device_transmit(spi_device, &t);
            if (ret == ESP_OK) {
                // Processa o Status (Primeiros 3 bytes)
                uint32_t status = (rx_buf[0] << 16) | (rx_buf[1] << 8) | rx_buf[2];
                
                // Processa o Canal 0 como exemplo (Bytes das posições 3, 4 e 5)
                int32_t ch0 = (rx_buf[3] << 16) | (rx_buf[4] << 8) | rx_buf[5];
                // Extensão de sinal para inteiros de 24 bits com sinal
                if (ch0 & 0x800000) ch0 |= 0xFF000000;

                ESP_LOGI(TAG, "Status: 0x%06lX | Ch0: %ld", status, ch0);
            } else {
                ESP_LOGE(TAG, "Erro na leitura SPI");
            }
        }
    }
}

void app_main(void) {
    init_gpio_control();
    init_spi();

    // Cria a tarefa de leitura com prioridade alta
    xTaskCreate(ads131_task, "ads131_task", 4096, NULL, 10, NULL);
}
