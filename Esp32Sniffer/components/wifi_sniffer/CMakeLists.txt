idf_component_register(SRCS  "console_sniffer_cmd.c" "capture_lib.c" "wifi_sniffer.c" "wifi_sniffer_nvs.c"
                    INCLUDE_DIRS "include"
                    REQUIRES console_lib wifi_lib nvs_lib
                    )
