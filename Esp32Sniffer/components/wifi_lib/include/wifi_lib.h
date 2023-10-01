#ifndef F6F60399_78BC_4F19_BAA2_E8920D48BFD4
#define F6F60399_78BC_4F19_BAA2_E8920D48BFD4

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"

#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "lwip/ip_addr.h"

#include "wifi_lib_nvs.h"

typedef void (*wifi_lib_event_handler_t)(int32_t event_id, void *event_data);
void wifi_lib_register_event_handler(wifi_lib_event_handler_t event_handler);
ESP_EVENT_DECLARE_BASE(WIFI_LIB_EVENT);

typedef enum
{
    WIFI_LIB_GOT_IP,
    WIFI_LIB_JOINED_SSID,
    WIFI_LIB_LEFT_SSID,
    WIFI_LIB_HAS_SSID
} wifi_lib_event_types_t;

typedef struct
{
    uint8_t mac[6];
    wifi_lib_mode_t wifi_lib_mode;
    esp_ip4_addr_t ip;
    char ssid_name[SSID_SZ];
    uint8_t channel;
} wifi_lib_cfg_t;
wifi_lib_cfg_t wifi_lib_get_curr_config();
void wifi_init(wifi_lib_event_handler_t wifi_lib_event_handler);
void wifi_set_mode(wifi_lib_mode_t wifi_lib_mode);
typedef void (*onMessage)(char *msg);
void wifi_get_mac(uint8_t mac[]);
void wifi_get_ip(char msg[], size_t sz);
void wifi_conf_set_static_ip(esp_netif_ip_info_t ip);
void wifi_ap(const ssid_cfg_t ssid_cfg, uint8_t channel, esp_netif_ip_info_t ip, dns_servers_info_t dns_servers_info);
void wifi_sta(const ssid_cfg_t ssid_cfg_sta);

#endif /* F6F60399_78BC_4F19_BAA2_E8920D48BFD4 */
