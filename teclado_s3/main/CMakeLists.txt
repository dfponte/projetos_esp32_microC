idf_component_register(SRCS "main.c" "hid_keyboard.c"
                       INCLUDE_DIRS "." 
                       "../components/esp_tinyusb/include"        # Caminho para o esp_tinyusb.h
                       "../components/esp_tinyusb/tinyusb/src"    # Caminho para o hid_device.h
                       REQUIRES driver)

