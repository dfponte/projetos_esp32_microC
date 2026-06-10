idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_adc esp_http_client nvs_flash esp_wifi)
