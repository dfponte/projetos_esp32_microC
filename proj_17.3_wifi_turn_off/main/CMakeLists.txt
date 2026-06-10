
idf_component_register(
    SRCS "http_client.c" "main.c" "wifi.c" http_client.c
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash lwip 
    esp_http_client 
)
