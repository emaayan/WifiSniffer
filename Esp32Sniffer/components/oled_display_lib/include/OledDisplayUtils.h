#ifndef OLED_DISPLAY_UTILS_H
#define OLED_DISPLAY_UTILS_H

#include <stdio.h>


void initDisplay();
void displayPrint(uint8_t chXpos, uint8_t chYpos, const char *fmt, ...);
#endif