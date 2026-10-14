#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_lcd_panel_io.h>
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/i2c.h" // Driver I2C nativo do ESP-IDF

// --- CONFIGURAÇÃO DE PINOS SPI (DISPLAY) ---
#define LCD_HOST       SPI2_HOST
#define PIN_SCLK       12   // SCL da Tela
#define PIN_MOSI       11   // SDA da Tela
#define PIN_LCD_DC     9
#define PIN_LCD_CS     10
#define PIN_LCD_RST    3
#define PIN_BK_LIGHT   46

// --- CONFIGURAÇÃO DE PINOS I2C (SENSOR BH1750) ---
#define I2C_MASTER_NUM      I2C_NUM_0
#define PIN_I2C_SDA         4   // Conecte no SDA do BH1750 (Mude se necessário)
#define PIN_I2C_SCL         5   // Conecte no SCL do BH1750 (Mude se necessário)
#define BH1750_ADDR         0x23 // Endereço I2C padrão do BH1750 (pino ADDR no GND)
#define BH1750_CMD_START    0x10 // Modo de medição contínua em alta resolução (1 lux)

#define PIN_RELE            2   // GPIO do Relé

// --- DEFINIÇÃO DE CORES (RGB565) ---
#define COR_PRETO      0x0000
#define COR_BRANCO     0xFFFF
#define COR_CINZA      0x5AEB
#define COR_VERDE      0x07E0
#define COR_VERMELHO   0xF800

// Fonte 8x16 compacta (Mapeamento ASCII)
// --- VEJA OS DOIS COLCHETES [256][16] ADICIONADOS AQUI PARA CORRIGIR O ERRO ---
const uint8_t font8x16[256][16] = {
    ['0']={0x00,0x3c,0x66,0x66,0x6e,0x76,0x66,0x66,0x66,0x66,0x3c,0x00,0x00,0x00,0x00,0x00},
    ['1']={0x00,0x18,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,0x00,0x00,0x00},
    ['2']={0x00,0x3c,0x66,0x06,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00,0x00,0x00,0x00,0x00,0x00},
    ['3']={0x00,0x3c,0x66,0x06,0x06,0x1c,0x06,0x06,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00},
    ['4']={0x00,0x06,0x0e,0x1e,0x36,0x66,0x7f,0x06,0x06,0x0f,0x00,0x00,0x00,0x00,0x00,0x00},
    ['5']={0x00,0x7e,0x60,0x60,0x7c,0x06,0x06,0x06,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00},
    ['6']={0x00,0x3c,0x66,0x60,0x7c,0x66,0x66,0x66,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00},
    ['7']={0x00,0x7e,0x66,0x0c,0x0c,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00},
    ['8']={0x00,0x3c,0x66,0x66,0x3c,0x66,0x66,0x66,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00},
    ['9']={0x00,0x3c,0x66,0x66,0x66,0x3e,0x06,0x06,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00},
    ['R']={0x00,0x7c,0x66,0x66,0x7c,0x68,0x6c,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['E']={0x00,0x7e,0x60,0x60,0x78,0x60,0x60,0x60,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['L']={0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x62,0x7e,0x00,0x00,0x00,0x00,0x00,0x00},
    ['A']={0x00,0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['Y']={0x00,0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['O']={0x00,0x3c,0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['N']={0x00,0x66,0x76,0x7e,0x7e,0x6e,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['F']={0x00,0x7e,0x60,0x60,0x78,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['U']={0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00},
    ['X']={0x00,0x66,0x66,0x24,0x18,0x18,0x24,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['S']={0x00,0x3c,0x66,0x60,0x3c,0x06,0x06,0x66,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['I']={0x00,0x3c,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['T']={0x00,0x7e,0x5a,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['M']={0x00,0x66,0x7f,0x7f,0x6b,0x63,0x63,0x63,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    [':']={0x00,0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    [' ']={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};


// --- FUNÇÕES GRÁFICAS ---
void desenhar_retangulo(esp_lcd_panel_handle_t panel, int x_start, int y_start, int x_end, int y_end, uint16_t cor) {
    int largura = x_end - x_start;
    int altura = y_end - y_start;
    uint16_t *buffer = malloc(largura * altura * sizeof(uint16_t));
    if (buffer == NULL) return;

    uint16_t cor_spi = (cor << 8) | (cor >> 8); 
    for (int i = 0; i < largura * altura; i++) {
        buffer[i] = cor_spi;
    }
    esp_lcd_panel_draw_bitmap(panel, x_start, y_start, x_end, y_end, buffer);
    free(buffer);
}

void escrever_texto_16(esp_lcd_panel_handle_t panel, int x, int y, const char *msg, uint16_t cor_texto, uint16_t cor_fundo) {
    uint16_t buffer[8 * 16];
    uint16_t c_texto = (cor_texto << 8) | (cor_texto >> 8);
    uint16_t c_fundo = (cor_fundo << 8) | (cor_fundo >> 8);

    for (int n = 0; n < strlen(msg); n++) {
        char c = msg[n];
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 8; j++) {
                buffer[i * 8 + j] = (font8x16[(uint8_t)c][i] & (128 >> j)) ? c_texto : c_fundo;
            }
        }
        esp_lcd_panel_draw_bitmap(panel, x + (n * 8), y, x + (n * 8) + 8, y + 16, buffer);
    }
}

// --- INICIALIZAÇÃO DO I2C PARA O SENSOR ---
void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // 100 kHz
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // Envia comando para ligar e iniciar a leitura contínua do BH1750
    uint8_t cmd = BH1750_CMD_START;
    i2c_master_write_to_device(I2C_MASTER_NUM, BH1750_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
}

// --- FUNÇÃO PARA LER OS LUX DO BH1750 ---
int ler_bh1750_lux(void) {
    uint8_t dados[2];
    // Lê os 2 bytes brutos do sensor digital
    esp_err_t err = i2c_master_read_from_device(I2C_MASTER_NUM, BH1750_ADDR, dados, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        return -1; // Erro de comunicação I2C
    }
    // Conversão oficial do datasheet do BH1750: (MSB << 8 | LSB) / 1.2
    uint16_t nivel_bruto = (dados[0] << 8) | dados[1];
    return (int)(nivel_bruto / 1.2);
}

// --- MAIN APPLICATION ---
void app_main(void)
{
    // 1. Inicializar Barramento SPI (Display)
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SCLK, .mosi_io_num = PIN_MOSI, .miso_io_num = -1,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 240 * 320 * 2,
    };
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // 2. Configurar Driver ST7789
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_LCD_DC, .cs_gpio_num = PIN_LCD_CS, .pclk_hz = 10 * 1000 * 1000,
        .lcd_cmd_bits = 8, .lcd_param_bits = 8, .spi_mode = 0, .trans_queue_depth = 10,
    };
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST, .rgb_endian = LCD_RGB_ENDIAN_RGB, .bits_per_pixel = 16,
    };
    esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_disp_on_off(panel_handle, true);
    
    gpio_set_direction(PIN_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BK_LIGHT, 1);

    // Configurar pino do Relé
    gpio_set_direction(PIN_RELE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RELE, 0);

    // 3. Inicializar I2C para o Sensor BH1750
    i2c_master_init();
    vTaskDelay(pdMS_TO_TICKS(200)); // Aguarda estabilização do sensor

    // 4. Desenhar Layout Fixo da Interface
    desenhar_retangulo(panel_handle, 0, 0, 240, 320, COR_PRETO);
    desenhar_retangulo(panel_handle, 0, 0, 240, 40, 0x18C3); // Top bar azul
    escrever_texto_16(panel_handle, 56, 12, "SISTEMA ILUM.", COR_BRANCO, 0x18C3);

    escrever_texto_16(panel_handle, 20, 70, "LUMINOSIDADE:", COR_CINZA, COR_PRETO);

    int lux_anterior = -1;
    bool rele_anterior = false;
    char buffer_texto[32]; 

    // --- LOOP PRINCIPAL ---
    while(1) {
        // Realiza a leitura real em LUX do BH1750
        int valor_lux = ler_bh1750_lux();

        if (valor_lux >= 0) { // Se a leitura I2C for bem-sucedida
            
            // Condição solicitada: menor que 10 LUX liga o Relé
            bool status_rele = (valor_lux < 10);
            gpio_set_level(PIN_RELE, status_rele ? 1 : 0);

            // Atualização Dinâmica Numérica do Valor Real de Lux
            if (valor_lux != lux_anterior) {
                snprintf(buffer_texto, sizeof(buffer_texto), "  %d LUX   ", valor_lux);
                escrever_texto_16(panel_handle, 70, 100, buffer_texto, COR_BRANCO, COR_PRETO);
                lux_anterior = valor_lux;
            }

            // Atualização Dinâmica do Cartão do Relé (RELAY ON / RELAY OFF)
            if (status_rele != rele_anterior || lux_anterior == valor_lux) {
                if (status_rele) {
                    desenhar_retangulo(panel_handle, 15, 180, 225, 250, COR_VERMELHO);
                    escrever_texto_16(panel_handle, 72, 207, "RELAY ON", COR_BRANCO, COR_VERMELHO);
                } else {
                    desenhar_retangulo(panel_handle, 15, 180, 225, 250, COR_VERDE);
                    escrever_texto_16(panel_handle, 68, 207, "RELAY OFF", COR_PRETO, COR_VERDE);
                }
                rele_anterior = status_rele;
            }
        } else {
            // Caso ocorra erro de conexão física nos fios do I2C
            escrever_texto_16(panel_handle, 50, 100, "ERRO SENSOR I2C", COR_VERMELHO, COR_PRETO);
        }

        vTaskDelay(pdMS_TO_TICKS(250)); // Tempo mínimo recomendado para o BH1750
    }
}
