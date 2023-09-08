#include <led_gpio.h>

#include "driver/gpio.h"

static int _led_pin = 0;
void led_init(int pin)
{
    ESP_ERROR_CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
}

void led_set(uint8_t on)
{
    ESP_ERROR_CHECK(gpio_set_level(_led_pin, on ? 1 : 0));
}


