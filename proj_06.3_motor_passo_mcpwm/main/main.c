#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"

#define STEP_GPIO 4
#define DIR_GPIO  5
#define PASSOS_POR_VOLTA 1600

void app_main(void) {
    // 1. Configurar GPIO de Direção
    gpio_set_level(DIR_GPIO, 1);
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DIR_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // 2. Configurar Canal RMT (Gerador de Pulsos)
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = STEP_GPIO,
        .resolution_hz = 1000000, // 1MHz (1 tick = 1us)
        .mem_block_symbols = 64,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &motor_chan));

    // 3. Criar o Encoder de Cópia Nativo (Não gera erro de compilação)
    rmt_encoder_handle_t copy_encoder = NULL;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder));

    // 4. Habilitar Canal
    ESP_ERROR_CHECK(rmt_enable(motor_chan));

    while (1) {
        printf("Girando 360 graus...\n");
        gpio_set_level(DIR_GPIO, 1); // Sentido Horário

        // 5. Definir 1 pulso (500us HIGH, 500us LOW = 1000Hz)
        rmt_symbol_word_t pulso = {{{500, 1, 500, 0}}};

        // 6. Transmitir e repetir PASSOS_POR_VOLTA vezes
        rmt_transmit_config_t tx_config = {
            .loop_count = PASSOS_POR_VOLTA - 1, 
        };
        
        ESP_ERROR_CHECK(rmt_transmit(motor_chan, copy_encoder, &pulso, sizeof(pulso), &tx_config));
        
        // Aguarda o fim do movimento
        rmt_tx_wait_all_done(motor_chan, -1);

        printf("Fim da volta. Aguardando 2 segundos...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
