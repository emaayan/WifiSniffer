#include <wifi_lib.h>

#include "freertos/event_groups.h"
#include <stdint.h>

static const char *TAG = "WifiLib";
static EventGroupHandle_t s_wifi_event_group;

static esp_netif_t *_esp_netif = NULL;
static void set_net_if(esp_netif_t *esp_netif)
{
    _esp_netif = esp_netif;
}
static esp_netif_t *get_net_if()
{
    return _esp_netif;
}
static void destroy_net_if()
{
    esp_netif_t *esp_netif = get_net_if();
    if (esp_netif)
    {
        ESP_LOGI(TAG, "Destorying netif");
        esp_netif_destroy(esp_netif);
        set_net_if(NULL);
    }
}
static esp_err_t wifi_set_dns_server(esp_netif_t *netif, esp_ip4_addr_t addr, esp_netif_dns_type_t type)
{

    if (wifi_is_valid_ip(addr))
    {
        esp_netif_dns_info_t dns = {.ip.u_addr.ip4 = addr, dns.ip.type = IPADDR_TYPE_V4};
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
        ESP_LOGI(TAG, "Set DNS " IPSTR, IP2STR(&dns.ip.u_addr.ip4));
    }
    return ESP_OK;
}

static void wifi_conf_static_ip(esp_netif_ip_info_t ip, dns_servers_info_t dns_servers_info)
{
    esp_netif_t *netif = get_net_if();
    if (netif)
    {
        wifi_lib_mode_t wifi_lib_mode = wifi_nvs_get_mode();
        if (wifi_lib_mode == WIFI_LIB_MODE_AP)
        {
            ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));

            ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
            ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip));
            ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));

            char ip_addr[ADDRESS_SZ] = "";
            snprintf(ip_addr, sizeof(ip_addr), IPSTR, IP2STR((&ip.ip)));

            char netmask_addr[ADDRESS_SZ] = "";
            snprintf(netmask_addr, sizeof(netmask_addr), IPSTR, IP2STR((&ip.netmask)));

            char gw_addr[ADDRESS_SZ] = "";
            snprintf(gw_addr, sizeof(gw_addr), IPSTR, IP2STR((&ip.gw)));
            wifi_nvs_set_static_ip_info(ip_addr, netmask_addr, gw_addr);

            ESP_LOGI(TAG, "Success to set static ip: %s, netmask: %s , gw: %s", ip_addr, netmask_addr, gw_addr); // check this still works

            wifi_set_dns_server(netif, dns_servers_info.primary_dns, ESP_NETIF_DNS_MAIN);
            wifi_set_dns_server(netif, dns_servers_info.secondery_dns, ESP_NETIF_DNS_BACKUP);
        }
        else
        {
            ESP_LOGI(TAG, "Static IP can only be configured in AP mode");
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
#define WIFI_EVENT_AP_STOP_BIT BIT4
#define WIFI_EVENT_STA_STOP_BIT BIT5
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
    case WIFI_EVENT_STA_STOP:
    {
        ESP_LOGI(TAG, "STA Stopped");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_STOP_BIT);
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
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_AP_STOP_BIT);
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

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html
// https://esp32tutorials.com/esp32-static-fixed-ip-address-esp-idf/
// https://github.com/espressif/esp-idf/blob/master/examples/wifi/softap_sta/main/softap_sta.c

#define CONFIG_MAX_STA_CONN 2 // CONFIG_MAX_STA_CONN_TO_AP

static int wifi_generate_ssid(uint8_t *ssid_prefix, char *gen_ssid, size_t gen_ssid_sz)
{
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, mac));
    int sz = snprintf(gen_ssid, gen_ssid_sz, "%s_%02X_%02X_%02X", ssid_prefix, mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "SSID will be %s:  ", gen_ssid);
    return sz;
}
static esp_netif_t *wifi_soft_ap(ssid_cfg_t ssid_cfg, uint8_t channel)
{
    esp_netif_t *netif = esp_netif_create_default_wifi_ap();
    ESP_LOGI(TAG, "Created ap netif");
    wifi_config_t wifi_cfg = {.ap = {
                                  .channel = channel,
                                  .max_connection = CONFIG_MAX_STA_CONN,
                                  .authmode = ssid_cfg.pass_sz == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK,
                                  .pmf_cfg = {
                                      .required = false,
                                  }}};

    char gen_ssid[SSID_SZ] = "";
    int sz = wifi_generate_ssid(ssid_cfg.ssid, gen_ssid, sizeof(gen_ssid));

    memcpy(wifi_cfg.ap.ssid, gen_ssid, sz);
    wifi_cfg.ap.ssid_len = sz;
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
    ESP_LOGI(TAG, "Attempting to join %s:  ", staConf.bssid);
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

void wifi_get_mac(uint8_t mac[])
{
    ESP_ERROR_CHECK(esp_netif_get_mac(get_net_if(), mac));
}

void wifi_get_ip(char msg[], size_t sz)
{
    esp_netif_ip_info_t ipInfo;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(get_net_if(), &ipInfo));
    snprintf(msg, sz, "" IPSTR, IP2STR(&ipInfo.ip));
}

// static bool ap_mode_started = false;
void wifi_ap(const ssid_cfg_t ssid_cfg, uint8_t channel, esp_netif_ip_info_t ip, dns_servers_info_t dns_servers_info)
{
    ESP_LOGI(TAG, "Setting SoftAP");    
    ESP_ERROR_CHECK(esp_wifi_stop());
    destroy_net_if();
    ESP_LOGI(TAG, "Waiting for SoftAP to be stopped");
    EventBits_t bits = 0;   
    {        
        esp_netif_t *esp_netif = wifi_soft_ap(ssid_cfg, channel);
        ESP_ERROR_CHECK(esp_wifi_start());
        wifi_nvs_set_mode(WIFI_LIB_MODE_AP);
        set_net_if(esp_netif);

        ESP_LOGI(TAG, "Waiting for AP Mode");
        bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_EVENT_AP_START_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        if (bits & WIFI_EVENT_AP_START_BIT)
        {
            wifi_conf_static_ip(ip, dns_servers_info);
            char msg[50] = "";
            wifi_get_ip(msg, sizeof(msg));
            ESP_LOGI(TAG, "IP: %s", msg);
        }
        else
        {
            ESP_LOGE(TAG, "UNEXPECTED EVENT %" PRIu32, bits);
        }
    }
}

void wifi_sta(const ssid_cfg_t ssid_cfg_sta)
{
    ESP_LOGI(TAG, "Setting STA mode");    
    ESP_ERROR_CHECK(esp_wifi_stop());
    destroy_net_if();
    esp_netif_t *esp_netif = wifi_set_sta(ssid_cfg_sta);
    ESP_ERROR_CHECK(esp_wifi_start());
    set_net_if(esp_netif);

    ESP_LOGI(TAG, "Waiting for ESP to join");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_EVENT_STA_START_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_EVENT_STA_START_BIT)
    {
        ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_netif)); // MUST NOT HAVE DHCP SERVER WHEN JOINING A NETWORK
        ESP_ERROR_CHECK(esp_wifi_connect());
        wifi_nvs_set_mode(WIFI_LIB_MODE_STA);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT %" PRIu32, bits);
    }
}

void wifi_set_mode(wifi_lib_mode_t wifi_lib_mode)
{
    switch (wifi_lib_mode)
    {
    case WIFI_LIB_MODE_AP:
    {
        ESP_LOGI(TAG, "Settign mode AP");
        ssid_cfg_t ssid_cfg = wifi_nvs_get_ssid_sta_cfg();
        uint8_t channel = wifi_nvs_get_ap_channel();
        esp_netif_ip_info_t esp_netif_ip_info = wifi_nvs_get_static_ip_info();
        dns_servers_info_t dns_servers_info = wifi_nvs_get_dns_servers();
        wifi_ap(ssid_cfg, channel, esp_netif_ip_info, dns_servers_info);
    }
    break;
    case WIFI_LIB_MODE_STA:
    {
        ESP_LOGI(TAG, "Settign mode STA");
        ssid_cfg_t ssid_cfg = wifi_nvs_get_ssid_sta_cfg();
        wifi_sta(ssid_cfg);
        break;
    }
    case WIFI_LIB_MODE_NONE:
        ESP_LOGI(TAG, "Settign mode NONE");
        break;
    default:
        ESP_LOGE(TAG, "Unsupported Mode");
        break;
    }

    char msg[50] = "";
    wifi_get_ip(msg, sizeof(msg));
}
