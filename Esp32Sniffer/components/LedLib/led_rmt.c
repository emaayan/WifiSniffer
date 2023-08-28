#include "led_rmt.h"

#include "driver/gpio.h"
#include "led_strip.h"

static led_strip_handle_t led_strip;

void led_init(int pin)
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
}


#define R 16
#define G 16
#define B 16
void led_set(uint8_t s_led_state)
{
    /* If the addressable LED is enabled */
    if (s_led_state)
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, R, G, B));
        /* Refresh the strip to send data */
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    }
    else
    {
        /* Set all LED off to clear all pixels */
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
    }
}
