
#include "esp_log.h"

static const char *TAG = "MockLCD";
void display_lib_init()
{
   // ESP_LOGI(TAG, "Init Display");
}
void display_lib_print_big(uint8_t chXpos, uint8_t chYpos, const char *fmt, ...)
{
   // VA(LINE_SZ);
  //  ESP_LOGI(TAG, "%s", buffer);
}
void display_lib_print_std(uint8_t chXpos, uint8_t chYpos,uint32_t color, const char *fmt, ...)
{
  //  VA(LINE_SZ);
  //  ESP_LOGI(TAG, "%s", buffer);
}