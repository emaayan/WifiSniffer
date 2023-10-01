#include "wifi_lib_nvs.h"

#include <string.h>
#include "esp_log.h"
#include "lwip/ip_addr.h"

static const char *TAG = "WifiLibNvs";

static bool wifi_is_ip_equel(esp_ip4_addr_t esp_ip4_addr, esp_ip4_addr_t prev_esp_ip4_addr_t)
{
    return esp_ip4_addr.addr == prev_esp_ip4_addr_t.addr;
}
bool wifi_is_valid_ip(esp_ip4_addr_t esp_ip4_addr)
{
    return esp_ip4_addr.addr != IPADDR_NONE;
}
bool wifi_is_valid_ip_info(esp_netif_ip_info_t esp_netif_ip_info)
{
    return wifi_is_valid_ip(esp_netif_ip_info.ip) && wifi_is_valid_ip(esp_netif_ip_info.netmask) && wifi_is_valid_ip(esp_netif_ip_info.gw);
}

wifi_lib_mode_t wifi_nvs_set_mode(wifi_lib_mode_t value)
{
    wifi_lib_mode_t prev_wifi_lib_mode = wifi_nvs_get_mode();
    if (prev_wifi_lib_mode != value)
    {
        ESP_ERROR_CHECK(nvs_set_num32i(WIFI_NS, "mode", value));
    }
    else
    {
        ESP_LOGD(TAG, "Mode not changed");
    }
    return value;
}
wifi_lib_mode_t wifi_nvs_get_mode()
{
    int32_t value = CONFIG_DEF_MODE;
    nvs_get_num32i(WIFI_NS, "mode", &value, value);
    return value;
}

static void wifi_nvs_get_ssid(const char *key, char *value, const char *def_value, size_t sz)
{
    char v[SSID_SZ] = "";
    ESP_ERROR_CHECK(nvs_get_string(WIFI_NS, key, v, def_value, sizeof(v)));
    strncpy(value, v, sz);
}

static void wifi_nvs_get_ssid_pw(const char *key, char *value, const char *def_value, size_t sz)
{
    char v[SSID_PASS_SZ] = "";
    ESP_ERROR_CHECK(nvs_get_string(WIFI_NS, key, v, def_value, sizeof(v)));
    strncpy(value, v, sz);
}
static void wifi_nvs_set_ssid(const char *key, const char *value, size_t sz, const char *def_value)
{
    char v[SSID_SZ] = "";
    strncpy(v, value, sizeof(v));

    char prev[SSID_SZ] = "";
    wifi_nvs_get_ssid(key, prev, def_value, sz);
    if (strncmp(v, prev, sz))
    {
        ESP_ERROR_CHECK(nvs_set_string(WIFI_NS, key, v));
    }
    else
    {
        ESP_LOGD(TAG, "SSID not changed in NVS %s ", key);
    }
}
static void wifi_nvs_set_ssid_pw(const char *key, const char *value, size_t sz, const char *def_value)
{
    char v[SSID_PASS_SZ] = "";
    strncpy(v, value, sizeof(v));

    char prev[SSID_SZ] = "";
    wifi_nvs_get_ssid_pw(key, prev, def_value, sz);
    if (strncmp(v, prev, sz))
    {
        ESP_ERROR_CHECK(nvs_set_string(WIFI_NS, key, v));
    }
    else
    {
        ESP_LOGD(TAG, "SSID password not changed in NVS %s ", key);
    }
}

void wifi_nvs_set_ap_ssid(const char *value, size_t sz)
{
    wifi_nvs_set_ssid("ap-ssid", value, sz, CONFIG_DEF_AP_SSID);
}
void wifi_nvs_get_ap_ssid(char *value, size_t sz)
{
    wifi_nvs_get_ssid("ap-ssid", value, CONFIG_DEF_AP_SSID, sz);
}

void wifi_nvs_set_ap_ssid_pw(const char *value, size_t sz)
{
    wifi_nvs_set_ssid_pw("ap-ssid-pw", value, sz, CONFIG_DEF_AP_SSID_PW);
}
void wifi_nvs_get_ap_ssid_pw(char *value, size_t sz)
{
    wifi_nvs_get_ssid_pw("ap-ssid-pw", value, CONFIG_DEF_AP_SSID_PW, sz);
}

void wifi_nvs_set_sta_ssid(const char *value, size_t sz)
{
    wifi_nvs_set_ssid("sta-ssid", value, sz, CONFIG_DEF_STA_SSID);
}
void wifi_nvs_get_sta_ssid(char *value, size_t sz)
{
    wifi_nvs_get_ssid("sta-ssid", value, CONFIG_DEF_STA_SSID, sz);
}

void wifi_nvs_set_sta_ssid_pw(const char *value, size_t sz)
{
    wifi_nvs_set_ssid_pw("sta-ssid-pw", value, sz, CONFIG_DEF_STA_SSID_PW);
}
void wifi_nvs_get_sta_ssid_pw(char *value, size_t sz)
{
    wifi_nvs_get_ssid_pw("sta-ssid-pw", value, CONFIG_DEF_STA_SSID_PW, sz);
}

ssid_cfg_t convert_to_ssid_cfg(char *nvs_ssid, size_t nvs_ssid_sz, char *nvs_ssid_pw, size_t nvs_ssid_pw_sz)
{
    ssid_cfg_t ssid_cfg;
    strncpy((char *)ssid_cfg.ssid, nvs_ssid, nvs_ssid_sz);
    ssid_cfg.ssid_sz = strlen((char *)ssid_cfg.ssid);
    strncpy((char *)ssid_cfg.password, nvs_ssid_pw, nvs_ssid_pw_sz);
    ssid_cfg.pass_sz = strlen((char *)ssid_cfg.password);
    ESP_LOGI(TAG,"Using %s",ssid_cfg.ssid);
    return ssid_cfg;
}
ssid_cfg_t wifi_nvs_get_ssid_sta_cfg()
{
    char nvs_ssid[SSID_SZ] = "";
    wifi_nvs_get_sta_ssid(nvs_ssid, sizeof(nvs_ssid));
    ESP_LOGD(TAG, "ssid FROM nvs is %s", nvs_ssid);

    char nvs_ssid_pw[SSID_PASS_SZ] = "";
    wifi_nvs_get_sta_ssid_pw(nvs_ssid_pw, sizeof(nvs_ssid_pw));
    ESP_LOGD(TAG, "ssid pw FROM nvs is %s", nvs_ssid_pw);

    ssid_cfg_t ssid_cfg = convert_to_ssid_cfg(nvs_ssid, sizeof(nvs_ssid), nvs_ssid_pw, sizeof(nvs_ssid_pw));
    return ssid_cfg;
}
ssid_cfg_t wifi_nvs_get_ssid_ap_cfg()
{
    char nvs_ssid[SSID_SZ] = "";
    wifi_nvs_get_ap_ssid(nvs_ssid, sizeof(nvs_ssid));
    ESP_LOGD(TAG, "ssid FROM nvs is %s", nvs_ssid);

    char nvs_ssid_pw[SSID_PASS_SZ] = "";
    wifi_nvs_get_ap_ssid_pw(nvs_ssid_pw, sizeof(nvs_ssid_pw));
    ESP_LOGD(TAG, "ssid pw FROM nvs is %s", nvs_ssid_pw);

    ssid_cfg_t ssid_cfg = convert_to_ssid_cfg(nvs_ssid, sizeof(nvs_ssid), nvs_ssid_pw, sizeof(nvs_ssid_pw));
    return ssid_cfg;
}

void wifi_nvs_set_ap_channel(const uint8_t channel)
{
    const uint8_t prev_channel = wifi_nvs_get_ap_channel();
    if (prev_channel != channel)
    {
        ESP_ERROR_CHECK(nvs_set_num32i(WIFI_NS, "ap-channel", channel));
    }
    else
    {
        ESP_LOGD(TAG, "Channel not saved");
    }
}
uint8_t wifi_nvs_get_ap_channel()
{
    int32_t channel = CONFIG_ESP_WIFI_CHANNEL; // because default is saved as i32
    nvs_get_num32i(WIFI_NS, "ap-channel", &channel, CONFIG_ESP_WIFI_CHANNEL);
    return channel;
}

static esp_netif_ip_info_t convert_to_ip_info(const char *ip_address, const char *netmask, const char *gw)
{
    esp_netif_ip_info_t ip = {.ip.addr = ipaddr_addr(ip_address), .netmask.addr = ipaddr_addr(netmask), .gw.addr = ipaddr_addr(gw)};
    if (wifi_is_valid_ip_info(ip))
    {
        ESP_LOGD(TAG, "Converting to " IPSTR " from %s", IP2STR(&ip.ip), ip_address);
    }
    else
    {
        ESP_LOGE(TAG, "Error in IP,netmask,gw: %s %s %s", ip_address, netmask, gw);
    }

    return ip;
}

esp_netif_ip_info_t convert_to_ip(const char *ip_address)
{
    return convert_to_ip_info(ip_address, CONFIG_STATIC_NETMASK_ADDR, ip_address);
}

dns_servers_info_t convert_to_dns_servers(const char *primary_dns, const char *second_dns)
{
    dns_servers_info_t dns_servers_info = {.primary_dns.addr = ipaddr_addr(primary_dns), .secondery_dns.addr = ipaddr_addr(second_dns)};
    return dns_servers_info;
}

dns_servers_info_t wifi_nvs_set_dns_servers(const char *primary_dns, const char *second_dns)
{
    dns_servers_info_t dns_servers_info = convert_to_dns_servers(primary_dns, second_dns);
    dns_servers_info_t prev_dns_servers_info = wifi_nvs_get_dns_servers();
    if (strlen(primary_dns) && wifi_is_valid_ip(dns_servers_info.primary_dns))
    {
        if (!wifi_is_ip_equel(dns_servers_info.primary_dns, prev_dns_servers_info.primary_dns))
        {
            ESP_ERROR_CHECK(nvs_set_string(WIFI_NS, "dns1", primary_dns));
        }
        else
        {
            ESP_LOGD(TAG, "Primary DNS not changed");
        }
    }
    else
    {
        ESP_LOGE(TAG, "cannot save Primary DNS server - invalid value %s", primary_dns);
    }
    if (strlen(second_dns) && wifi_is_valid_ip(dns_servers_info.secondery_dns))
    {
        if (!wifi_is_ip_equel(dns_servers_info.secondery_dns, prev_dns_servers_info.secondery_dns))
        {
            ESP_ERROR_CHECK(nvs_set_string(WIFI_NS, "dns2", second_dns));
        }
        else
        {
            ESP_LOGD(TAG, "Secondery DNS not changed");
        }
    }
    else
    {
        ESP_LOGE(TAG, "cannot save Secondery DNS server - invalid value %s", primary_dns);
    }
    return dns_servers_info;
}

dns_servers_info_t wifi_nvs_get_dns_servers()
{
    char primary_dns[ADDRESS_SZ] = "";
    ESP_ERROR_CHECK(nvs_get_string(WIFI_NS, "dns1", primary_dns, CONFIG_MAIN_DNS_SERVER, sizeof(primary_dns)));

    char second_dns[ADDRESS_SZ] = "";
    ESP_ERROR_CHECK(nvs_get_string(WIFI_NS, "dns2", second_dns, CONFIG_BACKUP_DNS_SERVER, sizeof(second_dns)));

    dns_servers_info_t dns_servers_info = convert_to_dns_servers(primary_dns, second_dns);
    ESP_LOGI(TAG, "Got DNS " IPSTR "/" IPSTR, IP2STR(&dns_servers_info.primary_dns), IP2STR(&dns_servers_info.secondery_dns));
    return dns_servers_info;
}

static bool wifi_nvs_static_ip_equel(esp_netif_ip_info_t esp_netif_ip_info, esp_netif_ip_info_t prev_esp_netif_ip_info)
{
    return wifi_is_ip_equel(esp_netif_ip_info.ip, prev_esp_netif_ip_info.ip) && wifi_is_ip_equel(esp_netif_ip_info.netmask, prev_esp_netif_ip_info.netmask) && wifi_is_ip_equel(esp_netif_ip_info.gw, prev_esp_netif_ip_info.gw);
}
void wifi_nvs_set_static_ip_info(const char *ip_address, const char *netmask, const char *gw)
{

    esp_netif_ip_info_t esp_netif_ip_info = convert_to_ip_info(ip_address, netmask, gw);

    if (wifi_is_valid_ip_info(esp_netif_ip_info))
    {
        esp_netif_ip_info_t prev_esp_netif_ip_info = wifi_nvs_get_static_ip_info();
        if (!wifi_nvs_static_ip_equel(esp_netif_ip_info, prev_esp_netif_ip_info))
        {
            ESP_ERROR_CHECK(nvs_set_string(WIFI_NS, "ip", ip_address));
            ESP_ERROR_CHECK(nvs_set_string(WIFI_NS, "netmask", netmask));
            ESP_ERROR_CHECK(nvs_set_string(WIFI_NS, "gw", gw));
        }
        else
        {
            ESP_LOGD(TAG, "Same static IP");
        }
    }
    else
    {
        ESP_LOGE(TAG, "cannot save static ip config - invalid values");
    }
}
void wifi_nvs_set_static_ip(const char *ip_address)
{
    wifi_nvs_set_static_ip_info(ip_address, "255.255.255.0", ip_address);
}

esp_netif_ip_info_t wifi_nvs_get_static_ip_info()
{

    char ip_address[ADDRESS_SZ] = "";
    char netmask[ADDRESS_SZ] = "";
    char gw[ADDRESS_SZ] = "";

    ESP_ERROR_CHECK(nvs_get_string(WIFI_NS, "ip", ip_address, CONFIG_STATIC_IP_ADDR, sizeof(ip_address)));
    ESP_ERROR_CHECK(nvs_get_string(WIFI_NS, "netmask", netmask, CONFIG_STATIC_NETMASK_ADDR, sizeof(netmask)));
    ESP_ERROR_CHECK(nvs_get_string(WIFI_NS, "gw", gw, CONFIG_STATIC_GW_ADDR, sizeof(gw)));
    esp_netif_ip_info_t ip = convert_to_ip_info(ip_address, netmask, gw);
    return ip;
}
