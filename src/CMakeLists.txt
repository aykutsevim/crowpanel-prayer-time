# The firmware's main component (PlatformIO compiles lib/ and LVGL separately).
# REQUIRES lists exactly the ESP-IDF components the platform layer uses. Nothing audio-related is
# named here (constitution Principle I); tools/check_elf_silence.py proves the linked image holds
# no I2S, DAC or tone code.
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.cpp)
idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "platform"
    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_uart
        esp_lcd
        esp_timer
        esp_psram
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
        esp_http_client
        esp_http_server
        esp-tls
        mbedtls
        spi_flash
        espcoredump
        esp_partition
        joltwallet__littlefs
)
