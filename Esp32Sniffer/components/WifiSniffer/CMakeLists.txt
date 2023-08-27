idf_component_register(SRCS "console_utils.c" "console_sniffer_cmd.c" "console_lib.c" "led_lib.c" "capture_lib.c" "wifi_sniffer.c" "wifi_lib.c"
                    INCLUDE_DIRS "include"
                    REQUIRES console driver esp_wifi nvs_flash spi_flash
                    )
