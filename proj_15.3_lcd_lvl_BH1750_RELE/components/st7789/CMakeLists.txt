idf_component_register(
    SRCS 
        "st7789.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        esp_lcd 
        esp_lvgl_port 
        lvgl
        driver          # <-- ADICIONE ISSO AQUI para liberar driver/gpio.h e spi_master.h
)
