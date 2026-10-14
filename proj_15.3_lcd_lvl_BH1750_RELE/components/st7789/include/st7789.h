void func(void);
#ifndef ST7789_H
#define ST7789_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o display ST7789, barramento SPI e a engine do LVGL Port.
 */
void st7789_init(void);

#ifdef __cplusplus
}
#endif

#endif // ST7789_H
