#ifndef C570F7EA_080A_4C6C_861B_3A5E08C0FB26
#define C570F7EA_080A_4C6C_861B_3A5E08C0FB26

#include "nvs_lib.h"

#include "esp_netif_types.h"

#define WIFI_NS "settings"
#define ADDRESS_SZ 16
#define SSID_SZ 32
#define SSID_PASS_SZ 64

typedef enum
{
    WIFI_LIB_MODE_NONE = 0,
    WIFI_LIB_MODE_STA,
    WIFI_LIB_MODE_AP,
} wifi_lib_mode_t;


#define CONFIG_DEF_MODE WIFI_LIB_MODE_AP
#define CONFIG_DEF_AP_SSID CONFIG_SOFTAP_WIFI_SSID
#define CONFIG_DEF_AP_SSID_PW CONFIG_SOFTAP_WIFI_PASSWORD
#define CONFIG_ESP_WIFI_CHANNEL CONFIG_SOFTAP_WIFI_CHANNEL
#define CONFIG_STATIC_IP_ADDR CONFIG_SOFTAP_WIFI_IP
#define CONFIG_STATIC_GW_ADDR CONFIG_STATIC_IP_ADDR
#define CONFIG_STATIC_NETMASK_ADDR "255.255.255.0"
#define CONFIG_MAIN_DNS_SERVER ""   //"1.1.1.1"
#define CONFIG_BACKUP_DNS_SERVER "" //"8.8.8.8"
#define CONFIG_DEF_STA_SSID CONFIG_STA_WIFI_SSID
#define CONFIG_DEF_STA_SSID_PW CONFIG_STA_WIFI_PASSWORD


typedef struct
{
    uint8_t ssid[SSID_SZ];
    uint8_t ssid_sz;
    uint8_t password[SSID_PASS_SZ];
    uint8_t pass_sz;
} ssid_cfg_t;

typedef struct
{
    esp_ip4_addr_t primary_dns;
    esp_ip4_addr_t secondery_dns;
} dns_servers_info_t;


bool wifi_is_valid_ip(esp_ip4_addr_t esp_ip4_addr);

wifi_lib_mode_t wifi_nvs_set_mode(wifi_lib_mode_t wifi_lib_mode);
wifi_lib_mode_t wifi_nvs_get_mode();

void wifi_nvs_set_ap_ssid(const char *value, size_t sz);
void wifi_nvs_get_ap_ssid(char *value, size_t sz);

void wifi_nvs_set_ap_ssid_pw(const char *value, size_t sz);
void wifi_nvs_get_ap_ssid_pw(char *value, size_t sz);

void wifi_nvs_set_sta_ssid(const char *value, size_t sz);
void wifi_nvs_get_sta_ssid(char *value, size_t sz);

void wifi_nvs_set_sta_ssid_pw(const char *value, size_t sz);
void wifi_nvs_get_sta_ssid_pw(char *value, size_t sz);

ssid_cfg_t wifi_nvs_get_ssid_sta_cfg();
ssid_cfg_t wifi_nvs_get_ssid_ap_cfg();

void wifi_nvs_set_ap_channel(const uint8_t channel);
uint8_t wifi_nvs_get_ap_channel();

dns_servers_info_t wifi_nvs_set_dns_servers(const char *primary_dns, const char *second_dns);
dns_servers_info_t wifi_nvs_get_dns_servers();

void wifi_nvs_set_static_ip_info(const char *ip_address, const char *netmask, const char *gw);
void wifi_nvs_set_static_ip(const char *ip_address);
esp_netif_ip_info_t wifi_nvs_get_static_ip_info();

#endif /* C570F7EA_080A_4C6C_861B_3A5E08C0FB26 */
