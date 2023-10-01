#include <stdio.h>

#include "wifi_lib.h"
#include "wifi_sniffer.h"
#include "wifi_lib_nvs.h"

#include "console_lib.h"
#include "console_sniffer_cmd.h"
#include "console_wifi_cmd.h"

#include "tcp_server.h"
#include "serial_lib.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "led_common.h"

#include "OledDisplayUtils.h"
#include "lcd_lib.h"
#include "display_lib.h"

#include "iot_button.h"

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
    sniffer_packet_t sniffer_packet = sniffer_to_packet_data(pcap_rec);
    rssi_t rssi = sniffer_packet.rssi;
    uint32_t color = 0;
    if (rssi > -50)
    {
        color = LCD_BLUE;
    }
    else if (rssi > -70 && rssi < -51)
    {
        color = LCD_CYAN;
    }
    else
    {
        color = LCD_RED;
    }
    display_lib_print_std(0, 85, color, "%s, %d, %04x", sniffer_packet.ta, rssi, sniffer_packet.sq.seq); // TODO ADD ANOTHER TASK TO WRITE TO DISPLAY
    display_lib_print_std(0, 105, color, "a3: %s, fr: %04x", sniffer_packet.addr3,sniffer_packet.fctl); 

    return total_size;
}

void on_wifi_lib_event_handler(int32_t event_id, void *event_data);
void config_wifi_init()
{
    wifi_init(on_wifi_lib_event_handler);

    wifi_lib_mode_t wifi_lib_mode = wifi_nvs_get_mode(WIFI_LIB_MODE_AP);
    wifi_set_mode(wifi_lib_mode);
    // {
    //     char ssid[] = "MYAP";
    //     char pass[] = "catdocatdo";
    //     ssid_cfg_t ssid_cfg = convert_to_ssid_cfg(ssid, sizeof(ssid), pass, sizeof(pass));
    //     uint8_t channel = 2;
    //     esp_netif_ip_info_t esp_netif_ip_info = convert_to_ip("192.168.280.1");
    //     dns_servers_info_t dns_servers_info = convert_to_dns_servers("", "");
    //     wifi_ap(ssid_cfg, channel, esp_netif_ip_info, dns_servers_info);
    // }
    // {
    //     esp_netif_ip_info_t esp_netif_ip_info = convert_to_ip("192.1968.21.1");
    //     wifi_conf_set_static_ip(esp_netif_ip_info);
    // }
    // {
    //     char ssid[] = "MYSOFTAP";
    //     char pass[] = "catdjkjkocat";
    //     ssid_cfg_t ssid_cfg = convert_to_ssid_cfg(ssid, sizeof(ssid), pass, sizeof(pass));
    //     uint8_t channel = 2;
    //     esp_netif_ip_info_t esp_netif_ip_info = convert_to_ip("192.168.2.1");
    //     dns_servers_info_t dns_servers_info = convert_to_dns_servers("", "");
    //     wifi_ap(ssid_cfg, channel, esp_netif_ip_info, dns_servers_info);
    // }
    // {
    //     char ssid[] = "elhgroup";
    //     char pass[] = "9032";
    //     ssid_cfg_t ssid_cfg = convert_to_ssid_cfg(ssid, sizeof(ssid), pass, sizeof(pass));
    //     wifi_sta(ssid_cfg);
    // }
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

static void sniffer_display_filter()
{
    addrFilter_t addrFilter = sniffer_get_addr2_filter();
    rssi_t rssi = sniffer_get_rssi_filter();
    char mac_filter[20] = "";
    ADDR_TO_STRING(addrFilter, mac_filter);
    ESP_LOGI(TAG, "filter ,mac: %s ,rssi: %d ", mac_filter, rssi);
    display_lib_print_std(0, 62, LCD_CYAN, "filter ,mac: %s ,rssi: %d ", mac_filter, rssi);
}

void sniffer_on_event_handler(int32_t event_id, void *event_data)
{
    // ESP_LOGI(TAG, "Got Event %" PRId32, event_id);
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
    case SNIFFER_EVENT_FILTER_CHANGED:
        sniffer_display_filter();
        break;
    default:
        ESP_LOGW(TAG, "Unknown Event %" PRId32, event_id);
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

void display_init()
{
    display_lib_init();
}

static void sniffer_display(wifi_lib_cfg_t wifi_lib_cfg)
{
    ESP_LOGI(TAG, "%s, IP:" IPSTR " on channel %d", wifi_lib_cfg.ssid_name, IP2STR(&wifi_lib_cfg.ip), wifi_lib_cfg.channel);

    display_lib_print_std(0, 0, LCD_CYAN, "MAC: \t" MACSTR, MAC2STR(wifi_lib_cfg.mac));
    display_lib_print_std(0, 22, LCD_CYAN, "IP:  \t" IPSTR, IP2STR(&wifi_lib_cfg.ip));
    switch (wifi_lib_cfg.wifi_lib_mode)
    {
    case WIFI_LIB_MODE_AP:
        display_lib_print_std(0, 42, LCD_CYAN, "join :%s ,c:%d ", wifi_lib_cfg.ssid_name, wifi_lib_cfg.channel);
        break;
    case WIFI_LIB_MODE_STA:
        display_lib_print_std(0, 42, LCD_CYAN, "joined :%s ,c:%d ", wifi_lib_cfg.ssid_name, wifi_lib_cfg.channel);
        break;
    case WIFI_LIB_MODE_NONE:
        ESP_LOGE(TAG, "Unknown ");
        break;
    default:
        break;
    }
    sniffer_display_filter();
}

void on_wifi_lib_event_handler(int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Got Event %" PRId32, event_id);
    wifi_lib_cfg_t wifi_lib_cfg = wifi_lib_get_curr_config();
    switch (event_id)
    {
    case WIFI_LIB_GOT_IP:
        sniffer_display(wifi_lib_cfg);
        break;
    case WIFI_LIB_JOINED_SSID:
        sniffer_display(wifi_lib_cfg);
        break;
    case WIFI_LIB_LEFT_SSID:
        sniffer_display(wifi_lib_cfg);
        break;
    case WIFI_LIB_HAS_SSID:
        sniffer_display(wifi_lib_cfg);
        break;
    default:
        break;
    }
}

#define BUTTON_GPIO 14
static button_handle_t g_button;
static void button_single_click_cb(void *arg, void *data)
{
    button_event_t button_event = iot_button_get_event(arg);
    button_handle_t but_handle = (button_handle_t)arg;
    rssi_t rssi = sniffer_get_rssi_filter();
    if (rssi >= -50)
    {
        rssi = -70;
    }
    else if (rssi <= -51 && rssi >= -70)
    {
        rssi = -96;
    }
    else
    {
        rssi = -50;
    }
    sniffer_set_rssi_filter(rssi);
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");
}
void init_button()
{
#ifdef CONFIG_LILIGO_S3
    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = BUTTON_GPIO,
            .active_level = 0,
        },
    };
    g_button = iot_button_create(&cfg);
    ESP_ERROR_CHECK(iot_button_register_cb(g_button, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL));
#endif
}

void setup()
{
    ESP_LOGI(TAG, "[+] Startup...");

    nvs_init_flash();

    init_button();

    display_init();
    led_init_default();
    console_init();

    config_wifi_init();

    tcp_server();

    addrFilter_t ownMac = {.size = 6};
    wifi_get_mac(ownMac.addr);
    ESP_LOGI(TAG, "MAC Address Is : " MACSTR, MAC2STR(ownMac.addr));

    capture_set_cb(on_start_capture, on_capture);
    sniffer_init_config(ownMac, sniffer_on_event_handler);
    sniffer_set_no_filter();

    // addrFilter_t f = {{}, 0};
    // sniffer_set_addr2_filter(f);
#ifdef LILIGO_S3
    sniffer_start();
#endif
}

void app_main(void)
{
    setup();
}
