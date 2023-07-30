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

#include "../../build/config/sdkconfig.h"

typedef struct
{
    uint8_t ssid[32];
    uint8_t ssid_sz;
    uint8_t password[64];
    uint8_t pass_sz;
} ssid_cfg_t;

void wifi_init();
typedef void(* onMessage)(char *msg);
void wifi_get_mac(uint8_t mac[]);
void wifi_get_ip(char msg[], size_t sz);
void wifi_get_ssid(ssid_cfg_t *ssid_cfg);
void wifi_softAP(const ssid_cfg_t ssid_cfg, uint8_t channel, const char *ip_address, const char *netmask, const char *gw, const char *primary_dns, const char *second_dns);
void wifi_sta(const ssid_cfg_t ssid_cfg_sta);

// void wifi_init_setup_config();

#endif /* F6F60399_78BC_4F19_BAA2_E8920D48BFD4 */
