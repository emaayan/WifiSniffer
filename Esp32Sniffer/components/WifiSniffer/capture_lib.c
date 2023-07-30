#include <capture_lib.h>
#include <sys/time.h>
#include "esp_log.h"
#include <inttypes.h>

// https://wiki.wireshark.org/Development/LibpcapFileFormat
#define MAX_LENGTH 700 // 2500

static const char *TAG = "CaptureLib";
typedef struct pcap_hdr_s
{
    const uint32_t magic_number;
    const uint16_t version_major;
    const uint16_t version_minor;
    const uint32_t thiszone; /* GMT to local correction */
    const uint32_t sigfigs;  /* accuracy of timestamps */
    const uint32_t snaplen;  /* max length of captured packets, in octets */
    uint32_t network;        /* data link type */
} pcap_hdr_t;
pcap_hdr_t pcap_hdr = {.magic_number = 0xa1b2c3d4, .version_major = 2, .version_minor = 4, .thiszone = 0, .sigfigs = 0, .snaplen = MAX_LENGTH, .network = 105};

typedef struct pcaprec_hdr_s
{
    const uint32_t ts_sec;   /* timestamp seconds */
    const uint32_t ts_usec;  /* timestamp microseconds */
    const uint32_t incl_len; /* number of octets of packet saved in file */
    const uint32_t orig_len; /* actual length of packet */
} pcaprec_hdr_t;

typedef struct pcaprec
{
    const pcaprec_hdr_t pcaprec_hdr;
    uint8_t buf[MAX_LENGTH];
} __attribute__((packed)) pcaprec_t;


void set_network_type(uint32_t type)
{
    pcap_hdr.network = type; // MABYE USED FOR OTHER TYPES?
}
static write_cb _write_cb = NULL;

void setWriteCb(write_cb cb)
{
    ESP_LOGI(TAG, "Setting Callback");
    _write_cb = cb;
}

int data_write(void *buffer, size_t size)
{
    if (_write_cb)
    {
        return _write_cb(buffer, size);
    }
    else
    {
        return 0;
    }
}

#include <string.h>
int send_msg(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    char buff[MAX_LENGTH] = "";
    size_t sz = vsnprintf(buff, sizeof(buff), fmt, argptr);
    va_end(argptr);
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);

    pcaprec_hdr_t pcaprec_hdr = {.ts_sec = tv_now.tv_sec, .ts_usec = tv_now.tv_usec, .incl_len = sz, .orig_len = -1};
    const size_t sz_h = sizeof(pcaprec_hdr);

    pcaprec_t pcaprec = {.pcaprec_hdr = pcaprec_hdr, .buf = {}};
    memcpy(pcaprec.buf, buff, pcaprec_hdr.incl_len);
    size_t total_size = sz_h + pcaprec_hdr.incl_len;
    return data_write((u_int8_t *)&pcaprec, total_size);

    // data_write((u_int8_t *)&pcaprec_hdr, sz_h);
    // data_write(buff, sz);
}

int send_v_msg(const char *fmt, va_list argptr)
{
    char buff[MAX_LENGTH] = "";
    size_t sz = vsnprintf(buff, sizeof(buff), fmt, argptr);

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    pcaprec_hdr_t pcaprec_hdr = {.ts_sec = tv_now.tv_sec, .ts_usec = tv_now.tv_usec, .incl_len = sz, .orig_len = -1};
    const size_t sz_h = sizeof(pcaprec_hdr);

    pcaprec_t pcaprec = {.pcaprec_hdr = pcaprec_hdr, .buf = {}};
    memcpy(pcaprec.buf, buff, pcaprec_hdr.incl_len);
    size_t total_size = sz_h + pcaprec_hdr.incl_len;
    return data_write((u_int8_t *)&pcaprec, total_size);

    // data_write((u_int8_t *)&pcaprec_hdr, sz_h);
    // data_write(buff, sz);
}

void addPacket(uint32_t len, uint8_t *buf)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    pcaprec_hdr_t pcaprec_hdr = {.ts_sec = tv_now.tv_sec, .ts_usec = tv_now.tv_usec, .incl_len = len > pcap_hdr.snaplen ? pcap_hdr.snaplen : len, .orig_len = len};

    const size_t sz = sizeof(pcaprec_hdr);

    pcaprec_t pcaprec = {.pcaprec_hdr = pcaprec_hdr, .buf = {}};
    memcpy(pcaprec.buf, buf, pcaprec_hdr.incl_len);
    size_t total_size = sz + pcaprec_hdr.incl_len;
    data_write((u_int8_t *)&pcaprec, total_size);

    // data_write((u_int8_t *)&pcaprec_hdr, sz);
    // data_write(buf, pcaprec_hdr.incl_len);
}

void startCapture()
{
    const size_t sz = sizeof(pcap_hdr);
    u_int8_t *buf = (u_int8_t *)&pcap_hdr;
    data_write(buf, sz);
}