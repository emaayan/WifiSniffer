#ifndef B87D334F_B89B_40D8_8ACA_ED768E355CE9
#define B87D334F_B89B_40D8_8ACA_ED768E355CE9

#include <stdint.h>

void display_lib_init();
void display_lib_print_big(uint8_t chXpos, uint8_t chYpos, const char *fmt, ...);
void display_lib_print_std(uint8_t chXpos, uint8_t chYpos, uint32_t color, const char *fmt, ...);

#endif /* B87D334F_B89B_40D8_8ACA_ED768E355CE9 */
