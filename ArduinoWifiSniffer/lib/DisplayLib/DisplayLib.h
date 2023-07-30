#ifndef A7924042_8800_499F_B469_C7EF0C7D2C25
#define A7924042_8800_499F_B469_C7EF0C7D2C25

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#define TFT_GREY 0x5AEB // New colour
TFT_eSPI tft = TFT_eSPI();
void initDisplay()
{
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(1);
}
void print(int16_t x, int16_t y, uint8_t font, uint16_t fgcolor, uint16_t bgcolor, uint8_t size, const char *fmt, ...)
{
    tft.setCursor(x, y, font);
    // Set the font colour to be white with a black background, set text size multiplier to 1
    tft.setTextColor(fgcolor, bgcolor);
    tft.setTextSize(size);
    const size_t sz = 50;
    char buffer[sz] = "";
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, argptr);
    va_end(argptr);
    tft.print(buffer);
}

#endif /* A7924042_8800_499F_B469_C7EF0C7D2C25 */
