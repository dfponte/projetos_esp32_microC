#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_lcd_panel_io.h>
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// Pinos ESP32-S3
#define LCD_HOST       SPI2_HOST
#define PIN_SCLK       12   //SCL
#define PIN_MOSI       11   //SDA
#define PIN_LCD_DC     9
#define PIN_LCD_CS     10
#define PIN_LCD_RST    3
#define PIN_BK_LIGHT   46

// Fonte 8x8 simples (Exemplo: Letra 'O', 'L', 'A', '!', 'S')
const uint8_t font8x8[][8] = {
    ['O']={0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['L']={0x06,0x06,0x06,0x06,0x06,0x06,0x7E,0x00},
    ['A']={0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00},
    ['!']={0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00},
    ['S']={0x3C,0x66,0x06,0x3C,0x60,0x66,0x3C,0x00},
};

// Função para desenhar a mensagem caractere por caractere
void escrever_texto(esp_lcd_panel_handle_t panel, int x, int y, const char *msg, uint16_t cor) {
    uint16_t buffer[8 * 8];
    for (int n = 0; n < strlen(msg); n++) {
        char c = msg[n];
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                buffer[i * 8 + j] = (font8x8[(uint8_t)c][i] & (1 << j)) ? cor : 0x0000;
            }
        }
        esp_lcd_panel_draw_bitmap(panel, x + (n * 8), y, x + (n * 8) + 8, y + 8, buffer);
    }
}
void app_main(void)
{
 // 0. Limpar a tela (Pintar tudo de preto)
 
  // 1. Configurar SPI
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SCLK, .mosi_io_num = PIN_MOSI, .miso_io_num = -1,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 240 * 320 * 2,
    };
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // 2. Configurar Interface e Driver (ST7789)
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

    // 3. Ligar o Display
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_disp_on_off(panel_handle, true);
    
    gpio_set_direction(PIN_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BK_LIGHT, 1);

    // Criamos um buffer para 20 linhas de uma vez para ser rápido
   uint16_t *clean_buf = malloc(240 * 20 * sizeof(uint16_t));
   memset(clean_buf, 0, 240 * 20 * sizeof(uint16_t)); // 0 = Preto
    for(int i = 0; i < 320; i+= 20) {
    esp_lcd_panel_draw_bitmap(panel_handle, 0, i, 240, i + 20, clean_buf);
    }
     free(clean_buf);

    // 4. MANDA A MENSAGEM
    escrever_texto(panel_handle, 100, 150, "OLA!", 0xFFFF); // Branco
    escrever_texto(panel_handle, 100, 170, "SOS", 0xF800);  // Vermelho
}
