#include <stdio.h>

#include "OledDisplayUtils.h"

#include "wifi_lib.h"
#include "wifi_sniffer.h"
#include "wifi_lib_nvs.h"

#include "console_lib.h"
#include "console_sniffer_cmd.h"
#include "console_wifi_cmd.h"

#include "tcp_server.h"
#include "serial_lib.h"
#include "capture_lib.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "capture_lib.h"
#include "led_common.h"

#include "../build/config/sdkconfig.h"
static const char *TAG = "WifiSnifferMain";

void signal_start()
{
    sniffer_start();
}

void signal_stop()
{
    sniffer_stop();
}

int _sock = 0;
void on_socket_accept_handler(const int sock, struct sockaddr_in *so_in)
{
    disconnect_socket(_sock);
    _sock = sock;
    sniffer_start();
}

int on_start_capture(pcap_hdr_t pcap_hdr)
{
    void *msg = &pcap_hdr;
    size_t sz = sizeof(pcap_hdr);
    if (_sock)
    {
        if (!onSend(_sock, msg, sz))
        {
            _sock = 0;
            sniffer_stop();
        }
    }
    return sz;
}

int on_capture(pcap_rec_t pcap_rec, size_t total_size)
{
    void *msg = &pcap_rec;
    if (_sock && pcap_rec.pcap_rec_hdr.orig_len > 0)
    {
        if (!onSend(_sock, msg, total_size))
        {
            _sock = 0;
            sniffer_stop();
        }
    }
    return total_size;
}

void init_serials()
{
    serial_begin_0(115200); // TODO do we still need this?
                            //  static txConfigStruct_t txConfig = {UART_NUM_0, 100, TX_TASK_SIZE, onMsgProduce};
                            //   createTxTask(&txConfig);

    // static rxConfigStruct_t rxConfig = {.port = UART_NUM_0, .wait = 10, .taskSize = RX_TASK_SIZE, .serial_reader = on_serial_read};
    // createRxTask(&rxConfig);
}

void config_wifi_init()
{
    wifi_init();
    wifi_lib_mode_t wifi_lib_mode = wifi_nvs_get_mode(WIFI_LIB_MODE_AP);
    wifi_set_mode(wifi_lib_mode);


    // wifi_set_mode(wifi_lib_mode);
}

#define PORT CONFIG_TCP_SERVER_PORT
#define KEEPALIVE_IDLE CONFIG_TCP_SERVER_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL CONFIG_TCP_SERVER_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT CONFIG_TCP_SERVER_KEEPALIVE_COUNT

void tcp_server()
{
    static tcp_server_config_t tcp_server_config = {.port = PORT, .keepIdle = KEEPALIVE_IDLE, .keepInterval = KEEPALIVE_INTERVAL, .keepCount = KEEPALIVE_COUNT, .on_socket_accept = on_socket_accept_handler};
    start_tcp_server(&tcp_server_config);
}

void queue_full_message()
{
    led_blink_fast();
}
void sniffer_is_up_message()
{
    led_blink_slow();
}

void sniffer_capture_started()
{
    led_blink_slow();
    led_blink_slow();
}

void sniffer_on_event_handler(int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Got Event %" PRId32, event_id);
    switch (event_id)
    {
    case SNIFFER_EVENT_QUEUE_FULL:
        queue_full_message();
        break;
    case SNIFFER_EVENT_IS_UP:
        sniffer_is_up_message();
        break;
    case SNIFFER_EVENT_CAPTURE_STARTED:
        sniffer_capture_started();
        break;
    default:
        break;
    }
}

void console_init()
{
    // console_config_init();
    // console_start();
    console_sniffer_register_cmd();
    console_wifi_register_cmd();

    console_begin();
}

void setup()
{
    ESP_LOGI(TAG, "[+] Startup...");

    nvs_init_flash();
    led_init_default();
    console_init();
    // init_serials();

    // initDisplay();
    // displayPrint(0, 17, "hello %s", "world");
    

    config_wifi_init();

    tcp_server();

    addrFilter_t ownMac = {.size = 6};
    wifi_get_mac(ownMac.addr);
    ESP_LOGI(TAG, "MAC Address Is : " MACSTR, MAC2STR(ownMac.addr));

    capture_set_cb(on_start_capture, on_capture);
    sniffer_init_config(ownMac, sniffer_on_event_handler);
    sniffer_set_no_filter();

    
    // addrFilter_t f={{0x00, 0x0C, 0XCC}, 3};
    addrFilter_t f = {{}, 0};
    sniffer_set_addr2_filter(f);
}

void app_main(void)
{
    setup();
}
