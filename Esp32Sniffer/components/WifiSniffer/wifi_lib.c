
#include <wifi_lib.h>
#include "freertos/event_groups.h"
#include <stdint.h>


static const char *TAG = "WifiLib";
static EventGroupHandle_t s_wifi_event_group;

static esp_err_t wifi_set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE))
    {
        esp_netif_dns_info_t dns = {.ip.u_addr.ip4.addr = addr, dns.ip.type = IPADDR_TYPE_V4};
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
        ESP_LOGI(TAG, "Set DNS " IPSTR, IP2STR(&dns.ip.u_addr.ip4));
    }
    return ESP_OK;
}

static void wifi_set_static_ip(esp_netif_t *netif, const char *ip_address, const char *netmask, const char *gw, const char *primary_dns, const char *second_dns)
{

    ESP_LOGI(TAG, "Init Wifi");
    esp_netif_ip_info_t ip;
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(ip_address);
    ip.netmask.addr = ipaddr_addr(netmask);
    ip.gw.addr = ipaddr_addr(gw);

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));
    ESP_LOGI(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", ip_address, netmask, gw);

    if (strlen(primary_dns))
    {
        ESP_LOGI(TAG, "Configuring DNS");
        ESP_ERROR_CHECK(wifi_set_dns_server(netif, ipaddr_addr(primary_dns), ESP_NETIF_DNS_MAIN));
        if (strlen(second_dns))
        {
            ESP_ERROR_CHECK(wifi_set_dns_server(netif, ipaddr_addr(second_dns), ESP_NETIF_DNS_BACKUP));
        }
    }
}
static int s_retry_num = 0;

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {

    case IP_EVENT_STA_GOT_IP:
    {
        s_retry_num = 0;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&event->ip_info.ip));
        break;
    }
    case IP_EVENT_AP_STAIPASSIGNED:
    {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
        ESP_LOGI(TAG, "IP assigned :" IPSTR, IP2STR(&event->ip));
        break;
    }
    default:
        ESP_LOGI(TAG, "IP Event %" PRIu32, event_id);
        break;
    }
}
// https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32/api-guides/wifi.html?highlight=setsockopt#esp32-wi-fi-event-description

#define WIFI_EVENT_AP_START_BIT BIT0
#define WIFI_EVENT_STA_CONNECTED_BIT BIT1
#define WIFI_EVENT_STA_DISCONNECTED_BIT BIT2
#define WIFI_EVENT_STA_START_BIT BIT3
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
    {
        ESP_LOGI(TAG, "STA Statred");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_START_BIT);

        break;
    }
    case WIFI_EVENT_AP_START:
    {
        ESP_LOGI(TAG, "Started soft AP");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_AP_START_BIT);

        break;
    }
    case WIFI_EVENT_AP_STOP:
    {
        ESP_LOGI(TAG, "Stopped Soft AP");
        break;
    }
    case WIFI_EVENT_AP_STACONNECTED:
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_STA_CONNECTED:
    {
        ESP_LOGI(TAG, "connect STA");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_CONNECTED_BIT);

        break;
    }
    case WIFI_EVENT_STA_DISCONNECTED:
    {
        ESP_LOGI(TAG, "disconnected STA");
        // esp_wifi_connect();
        // s_retry_num++;
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_DISCONNECTED_BIT);

        break;
    }
    default:
        ESP_LOGI(TAG, "STA Event %" PRIu32, event_id);
        break;
    }
}

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html
// https://esp32tutorials.com/esp32-static-fixed-ip-address-esp-idf/
// https://github.com/espressif/esp-idf/blob/master/examples/wifi/softap_sta/main/softap_sta.c

#define CONFIG_MAX_STA_CONN 2 // CONFIG_MAX_STA_CONN_TO_AP

static esp_netif_t *wifi_soft_ap(ssid_cfg_t ssid_cfg, uint8_t channel)
{
    esp_netif_t *netif = esp_netif_create_default_wifi_ap();
    wifi_config_t wifi_cfg = {.ap = {
                                  .channel = channel,
                                  .max_connection = CONFIG_MAX_STA_CONN,
                                  .authmode = ssid_cfg.pass_sz == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK,
                                  .pmf_cfg = {
                                      .required = false,
                                  }}};
    
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, mac));
    char gen_ssid[32]="";
    int sz=snprintf(gen_ssid,sizeof(gen_ssid),"%s_%02X_%02X_%02X",ssid_cfg.ssid, mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "SSID will be %s:  ",gen_ssid);

    memcpy(wifi_cfg.ap.ssid, gen_ssid, sz);
    wifi_cfg.ap.ssid_len = sz;// ssid_cfg.ssid_sz,
    memcpy(wifi_cfg.ap.password, ssid_cfg.password, ssid_cfg.pass_sz);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
    return netif;
}

static esp_netif_t *wifi_set_sta(ssid_cfg_t ssid_cfg)
{
    wifi_sta_config_t staConf = {
        .scan_method = WIFI_ALL_CHANNEL_SCAN,
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    };
    memcpy(staConf.ssid, ssid_cfg.ssid, ssid_cfg.ssid_sz);
    memcpy(staConf.password, ssid_cfg.password, ssid_cfg.pass_sz);
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    wifi_config_t cfg = {.sta = staConf};
    ESP_LOGI(TAG, "Attempting to join %s:  ",staConf.bssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    return netif;
}

void wifi_init()
{
    ESP_LOGI(TAG, "Init Wifi");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    

    s_wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, &instance_got_ip));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // allocate resource for WiFi driver
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

esp_netif_t *_esp_netif = NULL;
static void set_net_if(esp_netif_t *esp_netif)
{
    _esp_netif = esp_netif;
}

void wifi_get_mac(uint8_t mac[])
{
    ESP_ERROR_CHECK(esp_netif_get_mac(_esp_netif, mac));
}

void wifi_get_ip(char msg[], size_t sz)
{
    esp_netif_ip_info_t ipInfo;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(_esp_netif, &ipInfo));
    snprintf(msg, sz, "" IPSTR, IP2STR(&ipInfo.ip));
}

void wifi_get_ssid(ssid_cfg_t *ssid_cfg)
{
    wifi_mode_t wifi_mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));

    if (wifi_mode == WIFI_MODE_AP)
    {
        wifi_config_t wifi_config;
        ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
        wifi_ap_config_t ap = wifi_config.ap;
        ssid_cfg->ssid_sz = ap.ssid_len;
        memcpy(ssid_cfg->ssid, ap.ssid, ssid_cfg->ssid_sz);
        ssid_cfg->pass_sz = strlen((const char *)ap.password);
        memcpy(ssid_cfg->password, ap.password, ssid_cfg->pass_sz); // TODO: how to know pasword length?
    }
    if (wifi_mode == WIFI_MODE_STA)
    {
        wifi_config_t wifi_config;
        ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_AP, &wifi_config));
        wifi_sta_config_t sta = wifi_config.sta;
        ssid_cfg->ssid_sz = strlen((const char *)sta.ssid);
        memcpy(ssid_cfg->ssid, sta.ssid, ssid_cfg->ssid_sz);
        ssid_cfg->pass_sz = strlen((const char *)sta.password);      // ssid_cfg.ssid_sz;
        memcpy(ssid_cfg->password, sta.password, ssid_cfg->pass_sz); // TODO: how to know pasword length?
    }
}

void wifi_softAP(const ssid_cfg_t ssid_cfg, uint8_t channel, const char *ip_address, const char *netmask, const char *gw, const char *primary_dns, const char *second_dns)
{

    ESP_LOGI(TAG, "Setting SoftAP");
    ESP_ERROR_CHECK(esp_wifi_stop());
    esp_netif_t *esp_netif = wifi_soft_ap(ssid_cfg, channel);
    ESP_ERROR_CHECK(esp_wifi_start());
    set_net_if(esp_netif);

    ESP_LOGI(TAG, "Holding For Event bits");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_EVENT_AP_START_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_EVENT_AP_START_BIT)
    {
        wifi_set_static_ip(esp_netif, ip_address, netmask, gw, primary_dns, second_dns);
        esp_netif_ip_info_t ipInfo;
        ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif, &ipInfo));
        char msg[50] = "";
        wifi_get_ip(msg, sizeof(msg));
        ESP_LOGI(TAG, "IP: %s", msg);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT %" PRIu32, bits);
    }
}

void wifi_sta(const ssid_cfg_t ssid_cfg_sta)
{
    ESP_LOGI(TAG, "Setting STA mode");
    ESP_ERROR_CHECK(esp_wifi_stop());
    esp_netif_t *esp_netif = wifi_set_sta(ssid_cfg_sta);
    ESP_ERROR_CHECK(esp_wifi_start());
    set_net_if(esp_netif);

    ESP_LOGI(TAG, "Holding For Event bits");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_EVENT_STA_START_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_EVENT_STA_START_BIT)
    {
        ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_netif)); // MUST NOT HAVE DHCP SERVER WHEN JOINING A NETWORK
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT %" PRIu32, bits);
    }
}

// #define CONFIG_ESP_WIFI_SSID CONFIG_SOFTAP_WIFI_SSID
// #define CONFIG_ESP_WIFI_PASS CONFIG_SOFTAP_WIFI_PASSWORD
// #define CONFIG_ESP_WIFI_CHANNEL CONFIG_SOFTAP_WIFI_CHANNEL
// #define CONFIG_STATIC_IP_ADDR CONFIG_SOFTAP_WIFI_IP
// #define CONFIG_STATIC_GW_ADDR CONFIG_STATIC_IP_ADDR
// #define CONFIG_STATIC_NETMASK_ADDR "255.255.255.0"
// #define CONFIG_MAIN_DNS_SERVER ""   //"1.1.1.1"
// #define CONFIG_BACKUP_DNS_SERVER "" //"8.8.8.8"

// #define WIFI_SSID CONFIG_STA_WIFI_SSID// ""   // CONFIG_WIFI_SSID
// #define WIFI_PASS CONFIG_STA_WIFI_PASSWORD //"" // CONFIG_WIFI_PASSWORD //"" // TODO: replace
// void wifi_init_setup_config()
// {

//     wifi_init();

//     ssid_cfg_t ssid_cfg = {.ssid = CONFIG_ESP_WIFI_SSID, .ssid_sz = strlen(CONFIG_ESP_WIFI_SSID), .password = CONFIG_ESP_WIFI_PASS, .pass_sz = strlen(CONFIG_ESP_WIFI_PASS)};
//     wifi_softAP(ssid_cfg, CONFIG_ESP_WIFI_CHANNEL, CONFIG_STATIC_IP_ADDR, CONFIG_STATIC_GW_ADDR, CONFIG_STATIC_NETMASK_ADDR,CONFIG_MAIN_DNS_SERVER,CONFIG_BACKUP_DNS_SERVER);

//     ssid_cfg_t ssid_cfg_sta = {.ssid = WIFI_SSID, .ssid_sz = strlen(WIFI_SSID), .password = WIFI_PASS, .pass_sz = strlen(WIFI_PASS)};
//     wifi_sta(ssid_cfg_sta);
// }
