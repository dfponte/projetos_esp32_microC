cmake_minimum_required(VERSION 3.16)

# O segredo é que esta linha DEVE vir antes do include
set(EXTRA_COMPONENT_DIRS "components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(proj_15_1_SPI_I2C_Lib_lvgl)