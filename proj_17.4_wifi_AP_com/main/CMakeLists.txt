
idf_component_register(
    SRCS "main.c" "wifi.c" 
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash lwip 
    esp_http_client 
)
