#ifndef FBDBBFED_1F93_4B01_BD14_CA78B2A8298C
#define FBDBBFED_1F93_4B01_BD14_CA78B2A8298C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>


#include "esp_event.h"
#include "esp_wifi_types.h"

typedef struct
{
    uint16_t frag;
    uint16_t seq;
} seq_ctrl_t;
seq_ctrl_t get_seq(int16_t seqctl);

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
} __attribute__((packed)) wifi_mgmt_hdr_t;

typedef struct // any change in fields above need to change here
{
    int16_t fctl;
    int16_t duration;
    uint8_t ra[6];
    uint8_t ta[6];
    uint8_t sa[6];
    int16_t seqctl;
    uint8_t payload[100]; // place for actuall log
} __attribute__((packed)) log_hdr_t;

void sniffer_set_own_mac_filter(addrFilter_t addrFilter);
void sniffer_set_addr2_filter(addrFilter_t addrFilter);
void sniffer_set_addr3_filter(addrFilter_t addrFilter);
typedef void (*sniffer_event_handler_t)(int32_t event_id, void *event_data);
void sniffer_init_config(addrFilter_t ownMac, sniffer_event_handler_t sniffer_event_handler);
void sniffer_set_filter_channel(uint8_t channel);
void sniffer_start();
void sniffer_stop();

ESP_EVENT_DECLARE_BASE(SNIFFER_EVENT);
typedef enum
{
    SNIFFER_EVENT_QUEUE_FULL,
    SNIFFER_EVENT_CAPTURE_STARTED,
    SNIFFER_EVENT_CAPTURE_STOPPED,
    SNIFFER_EVENT_IS_UP
} sniffer_event_types_t;

void sniffer_register_event_handler(sniffer_event_handler_t event_handler);
void sniffer_set_filter_data();
void sniffer_set_no_filter();
int sniffer_to_string(wifi_promiscuous_pkt_t *pkt, char *buff, size_t sz);


#endif /* FBDBBFED_1F93_4B01_BD14_CA78B2A8298C */
