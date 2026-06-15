cmake_minimum_required(VERSION 3.16)

# Adicione esta linha LOGO ANTES do comando "include($ENV{IDF_PATH}/tools/cmake/project.cmake)"
set(EXCLUDE_COMPONENTS freemodbus)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(proj_10.3_RS485_COM_PC)
