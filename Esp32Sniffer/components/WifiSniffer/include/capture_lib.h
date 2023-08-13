#ifndef E6048912_5A40_4391_8553_587FB73E6A4C
#define E6048912_5A40_4391_8553_587FB73E6A4C

#include <stdio.h>
#include <inttypes.h>
#include "../../build/config/sdkconfig.h"
#define MAX_LENGTH CONFIG_SNIFFER_SNAP_LEN // 2500

typedef struct pcap_capture_header
{
    const uint32_t magic_number;
    const uint16_t version_major;
    const uint16_t version_minor;
    const uint32_t thiszone; /* GMT to local correction */
    const uint32_t sigfigs;  /* accuracy of timestamps */
    const uint32_t snaplen;  /* max length of captured packets, in octets */
    uint32_t network;        /* data link type */
} pcap_hdr_t;

typedef struct pcap_rec_header
{
    const uint32_t ts_sec;   /* timestamp seconds */
    const uint32_t ts_usec;  /* timestamp microseconds */
    const uint32_t incl_len; /* number of octets of packet saved in file */
    uint32_t orig_len; /* actual length of packet */
} pcap_rec_hdr_t;

size_t hex_to_byte_array(const char *hexArray, size_t hexLength, uint8_t byteArray[]);

typedef struct pcap_rec
{
    pcap_rec_hdr_t pcap_rec_hdr;
    uint8_t buf[MAX_LENGTH];
} __attribute__((packed)) pcap_rec_t;

typedef int (*on_start_capture_cb)(pcap_hdr_t pcap_hdr);
typedef int (*on_capture_cb)(pcap_rec_t pcaprec,size_t total_size);
void capture_set_cb(on_start_capture_cb on_start_capture_cb, on_capture_cb on_capture_cb);
void capture_start();

pcap_rec_t capture_create_packet(uint32_t len, uint8_t *buf);
int capture_on_send(pcap_rec_t pcap_rec);


#endif /* E6048912_5A40_4391_8553_587FB73E6A4C */
