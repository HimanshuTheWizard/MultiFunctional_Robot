file(GLOB_RECURSE SOURCES "Src/*.c")

idf_component_register(
    SRCS ${SOURCES}
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_wifi nvs_flash mqtt
    REQUIRES cJSON
    INCLUDE_DIRS "Inc"
)
