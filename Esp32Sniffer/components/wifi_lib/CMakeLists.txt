idf_component_register(SRCS "wifi_lib.c" "wifi_lib_nvs.c" "console_wifi_cmd.c" "console_wifi_set_ap_cmd.c" "console_wifi_set_ip_cmd.c" "console_wifi_set_sta_cmd.c"
                    INCLUDE_DIRS "include"
                    REQUIRES console_lib esp_wifi nvs_lib
                    )
