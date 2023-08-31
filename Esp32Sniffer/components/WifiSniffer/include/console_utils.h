#ifndef E6F2400F_DA31_4959_8BA4_3CCACA5B7549
#define E6F2400F_DA31_4959_8BA4_3CCACA5B7549

#include "esp_console.h"
#include "argtable3/argtable3.h"

int console_args_parse(int argc, char **argv, void **argtable, struct arg_end *arg_end);
void printfln(const char *fmt, ...);

void console_register_common();
void console_reset_argstr(struct arg_str *argstr);
void console_reset_argend(struct arg_end **argend);

#define CONSOLE_ARGS_PARSE(_ARGC, _ARGV, _ARGTABLE, _ARG_END)   \
    do                                                          \
    {                                                           \
                                                                \
        int nerrors = arg_parse((_ARGC), (_ARGV), (_ARGTABLE)); \
        if (nerrors != 0)                                       \
        {                                                       \
            arg_print_errors(stderr, (_ARG_END), _ARGV[0]);     \
            return nerrors;                                     \
        }                                                       \
    } while (0)

#endif /* E6F2400F_DA31_4959_8BA4_3CCACA5B7549 */
