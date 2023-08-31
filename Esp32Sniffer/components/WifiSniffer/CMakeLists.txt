idf_component_register(SRCS "console_utils.c" "console_sniffer_cmd.c" "console_lib.c" "console_wifi_cmd.c" "capture_lib.c" "nvs_lib.c" "wifi_sniffer.c" "wifi_lib.c"
                    INCLUDE_DIRS "include"
                    REQUIRES console driver esp_wifi nvs_flash spi_flash
                    )
