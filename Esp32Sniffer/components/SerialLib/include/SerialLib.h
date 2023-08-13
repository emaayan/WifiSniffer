#ifndef SERIAL_LIB_N
#define SERIAL_LIB_N

#include "driver/uart.h"

#define LOG_PORT UART_NUM_0
#define LOG_LINE_SZ 200
int serial_v_write(uart_port_t port, char buffer[], size_t sz, const char *fmt, va_list argptr);
int serial_log(const char *fmt, va_list argptr);

void serial_begin_0(int baud);
void serial_begin_2(int baud);
int serial_write_0(const int8_t *src, size_t size);
int serial_write_2(const int8_t *src, size_t size);

typedef struct
{
    int8_t *data; //[MAX_MSG_SIZE];
    size_t size;
} sent_message_t;

typedef void (*serial_write_h)(sent_message_t *serial_message);

typedef struct
{
    const uart_port_t port;
    const u_int16_t delay;
    const uint32_t taskSize;
    const serial_write_h msgProducerFromSource;

} txConfigStruct_t;

#define TX_TASK_SIZE configMINIMAL_STACK_SIZE *4
void createTxTask(txConfigStruct_t *txConfiguration);

#define RX_SZ 50
#define RX_TASK_SIZE configMINIMAL_STACK_SIZE *5

typedef struct
{
    uart_port_t srcPort;
    int8_t data[RX_SZ];
    size_t size;
} serial_messsage_t;

typedef void (*serial_reader_h)(serial_messsage_t serial_message);
typedef struct rxConfigStruct
{
    const uart_port_t port;
    const u_int16_t wait;
    const uint32_t taskSize;
    const serial_reader_h serial_reader;

} rxConfigStruct_t;

void createRxTask(rxConfigStruct_t *rxConfiguration);

#endif
