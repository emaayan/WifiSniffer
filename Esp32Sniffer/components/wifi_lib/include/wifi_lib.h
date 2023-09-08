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

void wifi_init();
void wifi_set_mode(wifi_lib_mode_t wifi_lib_mode);
typedef void (*onMessage)(char *msg);
void wifi_get_mac(uint8_t mac[]);
void wifi_get_ip(char msg[], size_t sz);
void wifi_ap(const ssid_cfg_t ssid_cfg, uint8_t channel, esp_netif_ip_info_t ip, dns_servers_info_t dns_servers_info);
void wifi_sta(const ssid_cfg_t ssid_cfg_sta);

// void wifi_init_setup_config();

#endif /* F6F60399_78BC_4F19_BAA2_E8920D48BFD4 */
