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
    int16_t fctl;     // frame control
    int16_t duration; // duration id
    uint8_t ra[6];    // Receiver address TO: (or BSSID in QS data Frame) (addr1)
    uint8_t ta[6];    // Transmitter address From: (orBSSID in managment)(addr2)
    uint8_t sa[6];    // filtering/source Address address BSSID in data (addr3)
    int16_t seqctl;   // sequence control
    uint8_t *payload; // network data(includes addr4)
} __attribute__((packed)) wifi_mgmt_hdr;


void sniffer_set_own_mac_filter(addrFilter_t addrFilter);
void sniffer_set_addr2_filter(addrFilter_t addrFilter);
void sniffer_set_addr3_filter(addrFilter_t addrFilter);
void sniffer_init_config(addrFilter_t ownMac);
// void wifi_sniffer_init();
void sniffer_set_filter_channel(uint8_t channel);
// void wifi_sniffer_deinit();
void sniffer_start();
void sniffer_stop();
wifi_mgmt_hdr * sniffer_get_wifi_mgmt_hdr(wifi_promiscuous_pkt_t *pkt);
#endif /* FBDBBFED_1F93_4B01_BD14_CA78B2A8298C */
