
#include "serial_lib.h"
#include <stdio.h>
#include "freertos/task.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define RX_BUF_SIZE 512

static const char *TAG = "SerialLib";


void serial_begin(uart_port_t port, int txPin, int rxPin, int baud)
{
    const uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(port, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));

    ESP_ERROR_CHECK(uart_set_pin(port, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}
static bool started_0 = false;
void serial_begin_0(int baud)
{
    serial_begin(UART_NUM_0, GPIO_NUM_1, GPIO_NUM_3, baud);
    started_0 = true;
}

int serial_write_0(const int8_t *src, size_t size)
{
    if (started_0)
    {
        return uart_write_bytes(UART_NUM_0, src, size);
    }
    else
    {
        ESP_LOGE(TAG, "Did not init port 0");
        return 0;
    }
}


static void tx_task(void *arg)
{
    // esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    const txConfigStruct_t *txConfig = arg;
    const uart_port_t port = txConfig->port;
    const uint16_t delay = txConfig->delay;
    while (1)
    {
        sent_message_t msg; // TODO: turns this into byte array

        msg.size = 0;
        txConfig->msgProducerFromSource(&msg);
        if (msg.data)
        {
            uart_write_bytes(port, msg.data, msg.size);
            // free(msg.data);
        }

        vTaskDelay(delay / portTICK_PERIOD_MS); // to avoid task reset
    }
}

void createTxTask(txConfigStruct_t *txConfiguration)
{
    xTaskCreate(tx_task, "uart_tx_task", txConfiguration->taskSize, txConfiguration, configMAX_PRIORITIES - 5, NULL);
}

static void rx_task(void *arg)
{
    const rxConfigStruct_t *rxConfiguration = arg;
    const uart_port_t port = rxConfiguration->port;
    const uint16_t wait = rxConfiguration->wait;

    while (1)
    {

        int8_t data[RX_SZ + 1] = {'\0'};
        const size_t sz = sizeof(data);
        const int rxBytes = uart_read_bytes(port, data, sz,pdMS_TO_TICKS(wait));

        if (rxBytes > 0)
        {
            if (rxConfiguration->serial_reader)
            {
                data[rxBytes] = 0;
                ESP_LOGD("SerialReaderTask", "Read %d bytes: '%s'", rxBytes, data);
                // ESP_LOG_BUFFER_HEXDUMP("Serial Reader", data, rxBytes, ESP_LOG_INFO);
                serial_messsage_t msg;
                msg.srcPort = port;
                msg.size = rxBytes;
                memcpy(msg.data, data, rxBytes);
                msg.data[rxBytes] = 0;
                rxConfiguration->serial_reader(msg);
            }
            else
            {
                ESP_LOGW("SerialReaderTask ", "No ReadHandler");
            }
        }
    }
}
void createRxTask(rxConfigStruct_t *rxConfiguration)
{
    xTaskCreate(rx_task, "uart_rx_task", rxConfiguration->taskSize, rxConfiguration, configMAX_PRIORITIES-10, NULL);
}