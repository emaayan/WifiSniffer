#include <stdio.h>

#include "OledDisplayUtils.h"
#include "wifi_lib.h"
#include "wifi_sniffer.h"
#include "tcp_server.h"
#include "SerialLib.h"
#include "capture_lib.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "capture_lib.h"

#include "../build/config/sdkconfig.h"
#include <led_lib.h>
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

addrFilter_t filterAddr(char mac[], const size_t readBytes)
{
    addrFilter_t filter = {{}, 0};
    if (readBytes > 0)
    {
        if (readBytes % 2 == 0)
        {
            ESP_LOGI(TAG, "read filter %s bytes read,%d", mac, readBytes);
            size_t t = hex_to_byte_array(mac, readBytes, filter.addr);
            filter.size = t;
            ESP_LOG_BUFFER_HEX(TAG, filter.addr, filter.size);
        }
        else
        {
            return filter;
        }
    }
    else
    {
        ESP_LOGI(TAG, "No filter");
    }
    return filter;
}

addrFilter_t read_filter(char *op, serial_messsage_t msg)
{
    char mac[12] = "";
    const size_t sz = msg.size - 2;
    strncpy(mac, op + 2, sz);
    addrFilter_t filt = filterAddr(mac, sz);
    return filt;
}

static const size_t OP_CODE_SIZE = 2;
bool isOpCode(const char buff[], const char op[])
{
    return !strncmp(buff, op, OP_CODE_SIZE);
}

void readMessage(serial_messsage_t msg)
{

    if (msg.size >= OP_CODE_SIZE)
    {

        char *op = (char *)msg.data;
        ESP_LOGD(TAG, "Got %s %d ", op, msg.size);
        if (isOpCode(op, "S0"))
        {
            ESP_LOGI(TAG, "Got stop capture");
            signal_stop();
        }

        if (isOpCode(op, "S1"))
        {
            ESP_LOGD(TAG, "Got Start capture");
            signal_start();
        }
        if (isOpCode(op, "FD"))
        {
            sniffer_set_filter_data();
        }
        if (isOpCode(op, "FA"))
        {
            sniffer_set_no_filter();
        }
        if (isOpCode(op, "FC")) // SHOULD ONLY HAPPEN ON SERIAL
        {
            const size_t sz = msg.size - 2;
            int ch;
            if (sz > 0)
            {
                char channel[2] = "";
                strncpy(channel, op + 2, sz);
                ch = atoi(channel);
            }
            else
            {
                ch = 0;
            }
            sniffer_set_filter_channel(ch);
        }

        if (isOpCode(op, "F3"))
        {
            addrFilter_t filt = read_filter(op, msg);
            if (filt.size >= 0)
            {
                sniffer_set_addr3_filter(filt);
            }
        }

        if (isOpCode(op, "F2"))
        {
            addrFilter_t filt = read_filter(op, msg);
            if (filt.size >= 0)
            {
                sniffer_set_addr2_filter(filt);
            }
        }
    }
}

void on_serial_read(serial_messsage_t serMsg)
{
    ESP_LOGD(TAG, "port: %d,Data  %s ", serMsg.srcPort, serMsg.data);
    readMessage(serMsg);    
}


void init_serials()
{
    serial_begin_0(115200); // TODO: make sure this sets back 250000
                            // static txConfigStruct_t txConfig = {UART_NUM_0, 100, TX_TASK_SIZE, onMsgProduce};
                            //  createTxTask(&txConfig);

    static rxConfigStruct_t rxConfig = {.port = UART_NUM_0, .wait = 10, .taskSize = RX_TASK_SIZE, .serial_reader = on_serial_read};
    createRxTask(&rxConfig);
}

#ifdef CONFIG_WIFI_SOFTAP

#define SNIFFER_WIFI_SSID CONFIG_SOFTAP_WIFI_SSID
#define SNIFFER_WIFI_PASS CONFIG_SOFTAP_WIFI_PASSWORD
#define CONFIG_ESP_WIFI_CHANNEL CONFIG_SOFTAP_WIFI_CHANNEL
#define CONFIG_STATIC_IP_ADDR CONFIG_SOFTAP_WIFI_IP
#define CONFIG_STATIC_GW_ADDR CONFIG_STATIC_IP_ADDR
#define CONFIG_STATIC_NETMASK_ADDR "255.255.255.0"
#define CONFIG_MAIN_DNS_SERVER ""   //"1.1.1.1"
#define CONFIG_BACKUP_DNS_SERVER "" //"8.8.8.8"

#endif

#ifdef CONFIG_WIFI_STA
#define SNIFFER_WIFI_SSID CONFIG_STA_WIFI_SSID
#define SNIFFER_WIFI_PASS CONFIG_STA_WIFI_PASSWORD
#endif
void init_config_wifi()
{
    wifi_init();
    ssid_cfg_t ssid_cfg = {.ssid = SNIFFER_WIFI_SSID, .ssid_sz = strlen(SNIFFER_WIFI_SSID), .password = SNIFFER_WIFI_PASS, .pass_sz = strlen(SNIFFER_WIFI_PASS)};
    // #ifndef CONFIG_WIFI_NONE
    //     ssid_cfg_t ssid_cfg = {.ssid = SNIFFER_WIFI_SSID, .ssid_sz = strlen(SNIFFER_WIFI_SSID), .password = SNIFFER_WIFI_PASS, .pass_sz = strlen(SNIFFER_WIFI_PASS)};
    // #endif

#ifdef CONFIG_WIFI_SOFTAP
    wifi_softAP(ssid_cfg, CONFIG_ESP_WIFI_CHANNEL, CONFIG_STATIC_IP_ADDR, CONFIG_STATIC_NETMASK_ADDR, CONFIG_STATIC_GW_ADDR, CONFIG_MAIN_DNS_SERVER, CONFIG_BACKUP_DNS_SERVER);
#endif

#ifdef CONFIG_WIFI_STA
    wifi_sta(ssid_cfg);
#endif

    char msg[50] = "";
    wifi_get_ip(msg, sizeof(msg));
    //  displayPrint(0, 17, "IP: %s", msg);
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

void flash_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void sniffer_on_event_handler(int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Got Event %" PRId32, event_id);
    switch (event_id)
    {
    case SNIFFER_EVENT_QUEUE_FULL:
        led_blink_fast();
        break;
    case SNIFFER_EVENT_IS_UP:
        led_blink_slow();
        break;
    case SNIFFER_EVENT_CAPTURE_STARTED:
        led_blink_slow();
        led_blink_slow();
        break;
    default:
        break;
    }
}
void setup()
{
    ESP_LOGI(TAG, "[+] Startup...");

    flash_init();
    led_init();

    init_serials();

    // initDisplay();
    // displayPrint(0, 17, "hello %s", "world");
    // ESP_ERROR_CHECK(esp_event_loop_create(event_handler, NULL)); // initialize (wifi) event handler

    init_config_wifi();
    tcp_server();

    addrFilter_t ownMac = {.size = 6};
    wifi_get_mac(ownMac.addr);
    ESP_LOGI(TAG, "MAC Address Is : " MACSTR, MAC2STR(ownMac.addr));

    capture_set_cb(on_start_capture, on_capture);
    sniffer_init_config(ownMac, sniffer_on_event_handler);
    sniffer_set_no_filter();

    // sniffer_filter_data();
    // addrFilter_t f={{0x00, 0x0C, 0XCC}, 3};
    addrFilter_t f = {{}, 0};
    sniffer_set_addr2_filter(f);
}

void app_main(void)
{
    setup();
}
