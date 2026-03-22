# Este arquivo DEVE estar em: components/lvgl/CMakeLists.txt
file(GLOB_RECURSE SOURCES "src/*.c")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "." "src"
                       REQUIRES esp_lcd)
