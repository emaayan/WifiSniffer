#ifndef CA3A7CEC_4399_4627_8D38_F1A3751EBB95
#define CA3A7CEC_4399_4627_8D38_F1A3751EBB95
#ifndef FBDBBFED_1F93_4B01_BD14_CA78B2A8298C
#define FBDBBFED_1F93_4B01_BD14_CA78B2A8298C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "esp_wifi_types.h"

typedef struct
{
    uint8_t addr[6];
    int size;
} addrFilter_t;

typedef struct
{
    uint8_t startChannel;
    uint16_t channelSwitchInterval;
} sniffer_config_t;

typedef struct
{
    int16_t fctl;      // frame control
    int16_t duration;  // duration id
    uint8_t da[6];     // receiver address(addr1)
    uint8_t sa[6];     // sender address(addr2)
    uint8_t bssid[6];  // filtering address(addr3)
    int16_t seqctl;    // sequence control
    uint8_t *payload; // network data(includes addr4)
} __attribute__((packed)) wifi_mgmt_hdr;

// void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);
typedef void (*wifiPacketHandler_t)(const wifi_promiscuous_pkt_type_t type, wifi_promiscuous_pkt_t *ppkt, wifi_mgmt_hdr *header);
wifiPacketHandler_t setPacketHander(wifiPacketHandler_t wifiPacketHandler);
void setAddrOwnMacFilter(addrFilter_t addrFilter);
void setAddr2Filter(addrFilter_t addrFilter);
void setAddr1Filter(addrFilter_t addrFilter);
void wifi_sniffer_init_config();
// void wifi_sniffer_init();
void set_filter_channel(uint8_t channel);
// void wifi_sniffer_deinit();
void sniffer_start();
void sniffer_stop();



#endif /* FBDBBFED_1F93_4B01_BD14_CA78B2A8298C */


#endif /* CA3A7CEC_4399_4627_8D38_F1A3751EBB95 */
