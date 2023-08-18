#include <led_lib.h>

#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUILD_IN_LED GPIO_NUM_2

void init_led()
{
    ESP_ERROR_CHECK(gpio_set_direction(BUILD_IN_LED, GPIO_MODE_OUTPUT));
}

void set_led(bool on)
{
    ESP_ERROR_CHECK(gpio_set_level(BUILD_IN_LED, on ? 1 : 0));
}

void led_blink(void *pvParams)
{    
    while (1)
    {
        set_led(true);
//        vTaskDelay(1000 / portTICK_RATE_MS);
        set_led(false);
    }
    vTaskDelete(NULL);
}
