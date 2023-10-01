#include "lcd_lib.h"
// #include "display_lib.h"
#include <LGFX_DisplayS3.hpp>
#include "esp_log.h"
static LGFX lcd;
#define LINE_SZ 60
static const char *TAG = "LCD";
extern "C"
{
    void display_lib_init()
    {
        if (lcd.init())
        {
            lcd.setRotation(1);
            lcd.setBrightness(128);
        }else{
            ESP_LOGE(TAG,"Failed to init lcd");
        }
    }

    static void display_lib_print(uint8_t chXpos, uint8_t chYpos, uint32_t color, lgfx::v1::GFXfont font, char buffer[])
    {
        char line[LINE_SZ] = "";
        snprintf(line, sizeof(line), "%-59s", buffer);
        lcd.setTextColor(color, 0);
        lcd.drawString(line, chXpos, chYpos, &font);
    }

    void display_lib_print_big(uint8_t chXpos, uint8_t chYpos, const char *fmt, ...)
    {
        VA(LINE_SZ);
        display_lib_print(chXpos, chYpos, 0x00FFFFU, fonts::FreeSans12pt7b, buffer);
    }

    void display_lib_print_std(uint8_t chXpos, uint8_t chYpos, uint32_t color, const char *fmt, ...)
    {
        VA(LINE_SZ);
        display_lib_print(chXpos, chYpos, color, fonts::DejaVu18, buffer);
    }
}