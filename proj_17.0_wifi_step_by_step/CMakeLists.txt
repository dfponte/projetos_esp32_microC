# CMakeLists.txt (DA RAIZ DO PROJETO)
cmake_minimum_required(VERSION 3.16)

# Força o framework a carregar os componentes de rede globalmente
set(components main esp_wifi nvs_flash esp_event esp_netif)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(proj_17.0_wifi_step_by_step)
