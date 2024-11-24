#ifndef FBDBBFED_1F93_4B01_BD14_CA78B2A8298C
#define FBDBBFED_1F93_4B01_BD14_CA78B2A8298C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "capture_lib.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

#include "wifi_sniffer_nvs.h"

typedef struct
{
    uint16_t frag;
    uint16_t seq;
} seq_ctrl_t;
seq_ctrl_t get_seq(int16_t seqctl);



typedef struct
{
    uint8_t startChannel;
    uint16_t channelSwitchInterval;
} sniffer_config_t;

#define FC_SUBTYPE(fc)      (((fc) >> 4) & 0xF)
#define FC_TO_DS(fc)        (((fc) >> 8) & 0x1)
#define FC_FROM_DS(fc)      (((fc) >> 9) & 0x1)

// Management Subtypes
#define WIFI_PKT_MGMT_SUBTYPE_ASSOC_REQ      0x0 // 0000
#define WIFI_PKT_MGMT_SUBTYPE_ASSOC_RESP     0x1 // 0001
#define WIFI_PKT_MGMT_SUBTYPE_REASSOC_REQ    0x2 // 0010
#define WIFI_PKT_MGMT_SUBTYPE_REASSOC_RESP   0x3 // 0011
#define WIFI_PKT_MGMT_SUBTYPE_PROBE_REQ      0x4 // 0100
#define WIFI_PKT_MGMT_SUBTYPE_PROBE_RESP     0x5 // 0101
#define WIFI_PKT_MGMT_SUBTYPE_TIMING_ADV     0x6 // 0110
#define WIFI_PKT_MGMT_SUBTYPE_BEACON         0x8 // 1000
#define WIFI_PKT_MGMT_SUBTYPE_ATIM           0x9 // 1001
#define WIFI_PKT_MGMT_SUBTYPE_DISASSOC       0xA // 1010
#define WIFI_PKT_MGMT_SUBTYPE_AUTH           0xB // 1011
#define WIFI_PKT_MGMT_SUBTYPE_DEAUTH         0xC // 1100
#define WIFI_PKT_MGMT_SUBTYPE_ACTION         0xD // 1101
#define WIFI_PKT_MGMT_SUBTYPE_ACTION_NOACK   0xE // 1110

// Control Subtypes
#define WIFI_PKT_CTRL_SUBTYPE_TRIGGER        0x2
#define WIFI_PKT_CTRL_SUBTYPE_WRAPPER        0x7
#define WIFI_PKT_CTRL_SUBTYPE_BAR            0x8
#define WIFI_PKT_CTRL_SUBTYPE_BA             0x9
#define WIFI_PKT_CTRL_SUBTYPE_PS_POLL        0xA
#define WIFI_PKT_CTRL_SUBTYPE_RTS            0xB
#define WIFI_PKT_CTRL_SUBTYPE_CTS            0xC
#define WIFI_PKT_CTRL_SUBTYPE_ACK            0xD
#define WIFI_PKT_CTRL_SUBTYPE_CF_END         0xE
#define WIFI_PKT_CTRL_SUBTYPE_CF_END_ACK     0xF

// Data Subtypes
#define WIFI_PKT_DATA_SUBTYPE_DATA           0x0
#define WIFI_PKT_DATA_SUBTYPE_DATA_CF_ACK    0x1
#define WIFI_PKT_DATA_SUBTYPE_DATA_CF_POLL   0x2
#define WIFI_PKT_DATA_SUBTYPE_DATA_CF_ACK_POLL 0x3
#define WIFI_PKT_DATA_SUBTYPE_NULL           0x4
#define WIFI_PKT_DATA_SUBTYPE_CF_ACK         0x5
#define WIFI_PKT_DATA_SUBTYPE_CF_POLL        0x6
#define WIFI_PKT_DATA_SUBTYPE_CF_ACK_POLL    0x7
#define WIFI_PKT_DATA_SUBTYPE_QOS_DATA       0x8
#define WIFI_PKT_DATA_SUBTYPE_QOS_DATA_CF_ACK 0x9
#define WIFI_PKT_DATA_SUBTYPE_QOS_DATA_CF_POLL 0xA
#define WIFI_PKT_DATA_SUBTYPE_QOS_DATA_CF_ACK_POLL 0xB
#define WIFI_PKT_DATA_SUBTYPE_QOS_NULL       0xC
#define WIFI_PKT_DATA_SUBTYPE_QOS_CF_POLL    0xE
#define WIFI_PKT_DATA_SUBTYPE_QOS_CF_ACK_POLL 0xF

//https://www.oreilly.com/library/view/80211-wireless-networks/0596100523/ch04.html

typedef struct
{
	int16_t fctl;     // frame control
    int16_t duration; // duration id	
} __attribute__((packed)) wifi_base_hdr_t;

/////Managment
typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t da[6];  
    uint8_t sa[6];  
    uint8_t bssid[6];    
    int16_t seqctl;   
    uint8_t *payload; 
} __attribute__((packed)) wifi_managment_hdr_t;


/////Data
typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t da[6];    
    uint8_t sa[6];    
    uint8_t bssid[6];   
    int16_t seqctl;  
    uint8_t *payload; 
} __attribute__((packed)) wifi_data_ibss_hdr_t;

typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t da[6]; //RA
    uint8_t ta[6]; //BSSID    
    uint8_t sa[6];   
    int16_t seqctl;  
    uint8_t *payload; 
} __attribute__((packed)) wifi_data_from_ap_hdr_t;

typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t ra[6]; //BSSID
    uint8_t sa[6]; //TA    
    uint8_t da[6];   
    int16_t seqctl;  
    uint8_t *payload; 
} __attribute__((packed)) wifi_data_to_ap_hdr_t;

typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t ra[6]; 
    uint8_t ta[6];     
    uint8_t da[6];   
    int16_t seqctl;
    uint8_t sa[6];  
    uint8_t *payload; 
} __attribute__((packed)) wifi_data_wds_hdr_t;


/////Ctrl
typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t ra[6]; 
    uint8_t ta[6];         
} __attribute__((packed)) wifi_ctrl_rts_hdr_t;

typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t ra[6];             
} __attribute__((packed)) wifi_ctrl_cts_hdr_t;


typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t ra[6];             
} __attribute__((packed)) wifi_ctrl_ack_hdr_t;

typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t bssid[6];             
    uint8_t ta[6];
} __attribute__((packed)) wifi_ctrl_ps_poll_hdr_t;

typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
    uint8_t ra[6];             
    uint8_t ta[6];
} __attribute__((packed)) wifi_ctrl_ba_hdr_t;//bar


typedef struct
{
    wifi_base_hdr_t wifi_base_hdr;    
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

typedef void (*sniffer_event_handler_t)(int32_t event_id, void *event_data);
void sniffer_register_event_handler(sniffer_event_handler_t event_handler);
ESP_EVENT_DECLARE_BASE(SNIFFER_EVENT);
typedef enum
{
    SNIFFER_EVENT_QUEUE_FULL,
    SNIFFER_EVENT_CAPTURE_STARTED,
    SNIFFER_EVENT_CAPTURE_STOPPED,
    SNIFFER_EVENT_IS_UP,
    SNIFFER_EVENT_FILTER_CHANGED
} sniffer_event_types_t;


void sniffer_set_filter_data();
void sniffer_set_no_filter();
int sniffer_to_string(wifi_promiscuous_pkt_t *pkt, char *buff, size_t sz);


void sniffer_set_own_mac_filter(addrFilter_t addrFilter);
void sniffer_set_addr2_filter(addrFilter_t addrFilter);
void sniffer_set_addr3_filter(addrFilter_t addrFilter);

void sniffer_init_config(addrFilter_t ownMac, sniffer_event_handler_t sniffer_event_handler);
void sniffer_set_filter_channel(uint8_t channel);
void sniffer_start();
void sniffer_stop();

typedef struct
{
    seq_ctrl_t sq;
    char ta[20];
    char addr3[20];
    rssi_t rssi;
    int16_t fctl;
    uint8_t channel;
} sniffer_packet_t;

sniffer_packet_t sniffer_to_packet_data(pcap_rec_t msg);
void sniffer_set_rssi_filter(rssi_t rssi);
addrFilter_t sniffer_get_addr2_filter();
rssi_t sniffer_get_rssi_filter();
#endif /* FBDBBFED_1F93_4B01_BD14_CA78B2A8298C */
