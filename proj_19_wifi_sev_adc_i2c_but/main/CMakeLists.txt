idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_http_client esp_wifi esp_event nvs_flash driver esp_adc)
