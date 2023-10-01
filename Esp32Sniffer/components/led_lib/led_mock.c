#include "led_lib.h"
#include "ESP_LOG.h"
const char *TAG = "LedMock";
void led_init(int pin)
{
    ESP_LOGW(TAG, "init led Mock!");
}

void led_set(uint8_t s_led_state)
{
    ESP_LOGI(TAG, "Got led state %d ", s_led_state);
}