#ifndef E6F2400F_DA31_4959_8BA4_3CCACA5B7549
#define E6F2400F_DA31_4959_8BA4_3CCACA5B7549

#include "esp_console.h"
#include "argtable3/argtable3.h"

int console_args_parse(int argc, char **argv, void **argtable, struct arg_end *arg_end);
void printfln(const char *fmt, ...);

void console_register_common();



#endif /* E6F2400F_DA31_4959_8BA4_3CCACA5B7549 */
