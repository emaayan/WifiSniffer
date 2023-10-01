idf_component_register(SRCS "led_lib.c" "led_rmt.c" "led_gpio.c" "led_common.c" "led_mock.c"
                    REQUIRES driver
                    INCLUDE_DIRS "include")
