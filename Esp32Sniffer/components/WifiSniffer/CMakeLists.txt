idf_component_register(SRCS "capture_lib.c" "wifi_sniffer.c" "wifi_lib.c" "tcp_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_wifi nvs_flash
                    )
