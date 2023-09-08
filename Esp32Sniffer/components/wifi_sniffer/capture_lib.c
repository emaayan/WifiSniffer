#include <capture_lib.h>
#include <sys/time.h>
#include "esp_log.h"
#include <inttypes.h>
#include <string.h>

// https://wiki.wireshark.org/Development/LibpcapFileFormat

static const char *TAG = "CaptureLib";

static pcap_hdr_t pcap_hdr = {.magic_number = 0xa1b2c3d4, .version_major = 2, .version_minor = 4, .thiszone = 0, .sigfigs = 0, .snaplen = MAX_LENGTH, .network = 127}; // LINKTYPE_IEEE802_11_RADIOTAP
static ieee80211_radiotap_header_t ieee80211_radiotap_header = {.it_version = 0, .it_len = sizeof(ieee80211_radiotap_header_t) + sizeof(r_tapdata_t), .it_present = IT_PRESENT};

static int hex_to_decimal(char hexChar)
{
    if (hexChar >= '0' && hexChar <= '9')
        return hexChar - '0';
    else if (hexChar >= 'a' && hexChar <= 'f')
        return hexChar - 'a' + 10;
    else if (hexChar >= 'A' && hexChar <= 'F')
        return hexChar - 'A' + 10;
    else
        return -1; // Invalid character
}

size_t hex_to_byte_array(const char *hexArray, size_t hexLength, uint8_t byteArray[])
{
    size_t i;
    for (i = 0; i < hexLength / 2; i++)
    {
        int highNibble = hex_to_decimal(hexArray[i * 2]);
        int lowNibble = hex_to_decimal(hexArray[i * 2 + 1]);

        if (highNibble == -1 || lowNibble == -1)
        {
            ESP_LOGE(TAG, "Invalid hexadecimal character");
            return -1;
        }
        else
        {
            byteArray[i] = (unsigned char)((highNibble << 4) | lowNibble);
        }
    }
    return i;
}

static on_start_capture_cb _on_start_capture_cb = NULL;
static on_capture_cb _on_capture_cb = NULL;
void capture_set_cb(on_start_capture_cb on_start_capture_cb, on_capture_cb on_capture_cb)
{
    ESP_LOGI(TAG, "Setting Callback");
    _on_start_capture_cb = on_start_capture_cb;
    _on_capture_cb = on_capture_cb;
}

size_t capture_get_size(pcap_rec_hdr_t pcap_rec_hdr)
{
    const size_t sz = sizeof(pcap_rec_hdr_t);
    const size_t total_size = sz + pcap_rec_hdr.incl_len;
    return total_size;
}

int capture_on_send(pcap_rec_t pcap_rec)
{
    if (_on_capture_cb)
    {
        return _on_capture_cb(pcap_rec, capture_get_size(pcap_rec.pcap_rec_hdr));
    }
    else
    {
        return 0;
    }
}

void capture_start()
{
    if (_on_start_capture_cb)
    {
        _on_start_capture_cb(pcap_hdr);
    }
}

pcap_rec_hdr_t capture_create_header(uint32_t len)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    pcap_rec_hdr_t pcap_rec_hdr = {.ts_sec = tv_now.tv_sec, .ts_usec = tv_now.tv_usec, .incl_len = len > pcap_hdr.snaplen ? pcap_hdr.snaplen : len, .orig_len = len};
    return pcap_rec_hdr;
}


r_tapdata_t capture_radio_tap(wifi_pkt_rx_ctrl_t ctrl)
{
    r_tapdata_t r_tapdata = {.noise = ctrl.noise_floor, .signal = ctrl.rssi, .antenna = ctrl.ant, .r_tapdata_channel = {.channel = ctrl.channel}};
    return r_tapdata;
}

pcap_rec_t capture_create_packet(uint32_t len, uint8_t *buf)
{    
    pcap_rec_hdr_t pcap_rec_hdr = capture_create_header(len+ieee80211_radiotap_header.it_len);
    pcap_rec_t pcap_rec = {.pcap_rec_hdr = pcap_rec_hdr, .ieee80211_radiotap_header = ieee80211_radiotap_header, .buf = {}};    
    memcpy(pcap_rec.buf, buf, pcap_rec_hdr.incl_len);
    return pcap_rec;
}

pcap_rec_t capture_create_pcap_record(wifi_promiscuous_pkt_t *pkt)
{
    uint32_t sig_packetLength = pkt->rx_ctrl.sig_len;
    uint8_t *payload = pkt->payload;
    pcap_rec_t pcap_rec = capture_create_packet(sig_packetLength, payload);

    wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
    pcap_rec.r_tapdata.noise = ctrl.noise_floor;
    pcap_rec.r_tapdata.signal = ctrl.rssi;
    pcap_rec.r_tapdata.antenna = ctrl.ant;
    pcap_rec.r_tapdata.r_tapdata_channel.channel = ctrl.channel;
    return pcap_rec;
}
