#ifndef LCD_LIB_H
#define LCD_LIB_H

#include <stdio.h>

const uint32_t LCD_CYAN = 0x00FFFFU;
const uint32_t LCD_RED = 0xFF0000U;
const uint32_t LCD_BLUE = 0x0000FFU;

#define VA(SIZE)                                    \
    char buffer[SIZE] = "";                         \
    va_list argptr;                                 \
    va_start(argptr, fmt);                          \
    vsnprintf(buffer, sizeof(buffer), fmt, argptr); \
    va_end(argptr);

#endif