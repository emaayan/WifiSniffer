#ifdef CONFIG_LILIGO_S3
	#include "iot_button.h"
#endif


#ifdef CONFIG_LILIGO_S3
#define BUTTON_GPIO 14
static button_handle_t g_button;
static void button_single_click_cb(void *arg, void *data)
{
    button_event_t button_event = iot_button_get_event(arg);
    button_handle_t but_handle = (button_handle_t)arg;
    rssi_t rssi = sniffer_get_rssi_filter();
    if (rssi >= -50)
    {
        rssi = -70;
    }
    else if (rssi <= -51 && rssi >= -70)
    {
        rssi = -96;
    }
    else
    {
        rssi = -50;
    }
    sniffer_set_rssi_filter(rssi);
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");
}
#endif

#ifdef CONFIG_LILIGO_S3
void init_button()
{
    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = BUTTON_GPIO,
            .active_level = 0,
        },
    };
    g_button = iot_button_create(&cfg);
    ESP_ERROR_CHECK(iot_button_register_cb(g_button, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL));
}
#endif