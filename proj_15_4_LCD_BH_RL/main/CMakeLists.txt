idf_component_register(
    SRCS "main.c" "term_80x80.c" "Inte_80x80.c" "btn_80x80.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_lcd esp_lvgl_port lvgl
)
