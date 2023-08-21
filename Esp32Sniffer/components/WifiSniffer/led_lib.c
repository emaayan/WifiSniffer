#include <led_lib.h>

#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUILD_IN_LED GPIO_NUM_2

// #define BUILD_IN_LED GPIO_NUM_8 // FOR RGB
// #include "led_strip.h"

void led_init()
{
    ESP_ERROR_CHECK(gpio_set_direction(BUILD_IN_LED, GPIO_MODE_OUTPUT));
}

void led_set(uint8_t on)
{
    ESP_ERROR_CHECK(gpio_set_level(BUILD_IN_LED, on ? 1 : 0));
}

void led_blink(uint8_t delay)
{
    led_set(false);
    led_set(true);
    vTaskDelay(pdMS_TO_TICKS(delay));
    led_set(false);
}

void led_blink_fast()
{
    led_blink(10);
}
void led_blink_slow()
{
    led_blink(200);
}
