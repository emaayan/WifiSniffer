#ifndef E6048912_5A40_4391_8553_587FB73E6A4C
#define E6048912_5A40_4391_8553_587FB73E6A4C
#include <stdio.h>

typedef int (*write_cb)(void * buffer , size_t size);
void setWriteCb(write_cb cb);
void addPacket(uint32_t len, uint8_t *buf);
void startCapture();
int send_msg(const char *fmt, ...);
int send_v_msg(const char *fmt, va_list argptr);
#endif /* E6048912_5A40_4391_8553_587FB73E6A4C */
