idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES protocol_examples_common esp_wifi nvs_flash 
                    esp_http_client
) 
