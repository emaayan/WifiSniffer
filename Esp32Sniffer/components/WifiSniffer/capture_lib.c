#include <capture_lib.h>
#include <sys/time.h>
#include "esp_log.h"
#include <inttypes.h>

// https://wiki.wireshark.org/Development/LibpcapFileFormat

static const char *TAG = "CaptureLib";

static pcap_hdr_t pcap_hdr = {.magic_number = 0xa1b2c3d4, .version_major = 2, .version_minor = 4, .thiszone = 0, .sigfigs = 0, .snaplen = MAX_LENGTH, .network = 105};

void set_network_type(uint32_t type)
{
    pcap_hdr.network = type; // MABYE USED FOR OTHER TYPES?
}

static on_start_capture_cb _on_start_capture_cb = NULL;
static on_capture_cb _on_capture_cb = NULL;

void capture_set_cb(on_start_capture_cb on_start_capture_cb, on_capture_cb on_capture_cb)
{
    ESP_LOGI(TAG, "Setting Callback");
    _on_start_capture_cb = on_start_capture_cb;
    _on_capture_cb = on_capture_cb;
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

    pcap_rec_hdr_t pcaprec_hdr = {.ts_sec = tv_now.tv_sec, .ts_usec = tv_now.tv_usec, .incl_len = sz, .orig_len = -1};
    const size_t sz_h = sizeof(pcaprec_hdr);

    // data_write((u_int8_t *)&pcaprec_hdr, sz_h);
    // return data_write(buff, sz);
    return 0;
}

int send_v_msg(const char *fmt, va_list argptr)
{
    char buff[MAX_LENGTH] = "";
    size_t sz = vsnprintf(buff, sizeof(buff), fmt, argptr);

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    pcap_rec_hdr_t pcap_rec_hdr = {.ts_sec = tv_now.tv_sec, .ts_usec = tv_now.tv_usec, .incl_len = sz, .orig_len = -1};
    const size_t sz_h = sizeof(pcap_rec_hdr);

    // pcaprec_t pcaprec = {.pcaprec_hdr = pcaprec_hdr, .buf = {}};
    // memcpy(pcaprec.buf, buff, pcaprec_hdr.incl_len);
    // size_t total_size = sz_h + pcaprec_hdr.incl_len;
    // return data_write((u_int8_t *)&pcaprec, total_size);

    // data_write((u_int8_t *)&pcaprec_hdr, sz_h);
    // return data_write(buff, sz);
    return 0;
}

size_t capture_get_size(pcap_rec_hdr_t pcaprec_hdr)
{
    const size_t sz = sizeof(pcaprec_hdr);
    size_t total_size = sz + pcaprec_hdr.incl_len;
    return total_size;
}

pcap_rec_hdr_t capture_create_header(uint32_t len)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    pcap_rec_hdr_t pcap_rec_hdr = {.ts_sec = tv_now.tv_sec, .ts_usec = tv_now.tv_usec, .incl_len = len > pcap_hdr.snaplen ? pcap_hdr.snaplen : len, .orig_len = len};
    return pcap_rec_hdr;
}

pcap_rec_t capture_create_packet(uint32_t len, uint8_t *buf)
{
    pcap_rec_hdr_t pcap_rec_hdr = capture_create_header(len);
    pcap_rec_t pcap_rec = {.pcap_rec_hdr = pcap_rec_hdr, .buf = {}};
    memcpy(pcap_rec.buf, buf, pcap_rec_hdr.incl_len);
    return pcap_rec;
}

int capture_on_send(pcap_rec_t pcap_rec)
{
    if (_on_capture_cb)
    {
        return _on_capture_cb(pcap_rec);
    }
    else
    {
        return 0;
    }
}

int capture_add_packet(uint32_t len, uint8_t *buf)
{
    pcap_rec_hdr_t pcap_rec_hdr = capture_create_header(len);
    pcap_rec_t pcap_rec = capture_create_packet(len, buf);
    return capture_on_send(pcap_rec);
}


void capture_start()
{
    if (_on_start_capture_cb)
    {
        _on_start_capture_cb(pcap_hdr);
    }
}