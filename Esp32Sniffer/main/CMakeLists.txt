idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "." REQUIRES console tcp_server wifi_sniffer oled_display_lib serial_lib led_lib)
