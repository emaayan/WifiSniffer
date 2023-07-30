#ifndef E653152C_730A_4ED6_9AA3_B1A780453B0A
#define E653152C_730A_4ED6_9AA3_B1A780453B0A

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define MAC_SIZE 6
typedef struct
{
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[MAC_SIZE]; /* receiver address */
  uint8_t addr2[MAC_SIZE]; /* sender address */
  uint8_t addr3[MAC_SIZE]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[MAC_SIZE]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct
{
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch (type)
  {
  case WIFI_PKT_MGMT:
    return "MGMT";
  case WIFI_PKT_DATA:
    return "DATA";
  case WIFI_PKT_MISC:
    return "MISC";
  default:
    return "";
  }
}

void wifi_sniffer_set_channel(uint8_t channel)
{
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

typedef void (*wifi_packet_cb)(wifi_promiscuous_pkt_type_t type, const wifi_promiscuous_pkt_t *ppkt, uint32_t packetLength);
wifi_packet_cb _w_cb = NULL;

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{
  // if (type != WIFI_PKT_MGMT)
  //   return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t*)buff;
  const wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)ppkt->rx_ctrl;
  uint32_t packetLength = ctrl.sig_len;
  if (type == WIFI_PKT_MGMT)
  {
    packetLength -= 4;
  }
  if (_w_cb)
  {
    _w_cb(type, ppkt, packetLength);
  }
}

esp_netif_t *sta_netif = NULL;
esp_netif_t *ap_netif = NULL;

static wifi_country_t wifi_country = {.cc = "CN", .schan = 1, .nchan = 13}; // Most recent esp32 library struct

#define WIFI_CHANNEL_SWITCH_INTERVAL (500)
#define WIFI_CHANNEL_MAX (13)

static const uint8_t channels[] = {1, 6, 11};
size_t chInx = 0;
uint8_t channel = channels[0];
uint8_t changeChannel()
{
  chInx = chInx >= sizeof(channels) - 1 ? 0 : chInx + 1;
  channel = channels[chInx];
  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
  wifi_sniffer_set_channel(channel);
  return channel;
  // channel = (channel % WIFI_CHANNEL_MAX) + 1;
}

void wifi_sniffer_init(wifi_packet_cb callback)
{
  nvs_flash_init();
  esp_netif_init();
  sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country)); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_set_promiscuous(true);
  _w_cb = callback;
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

const uint32_t magic_number = 0xa1b2c3d4;
const uint16_t version_major = 2;
const uint16_t version_minor = 4;
const uint32_t thiszone = 0;
const uint32_t sigfigs = 0;
const uint32_t snaplen = 2500;
const uint32_t network = 105;

typedef void (*write_cb)(const uint8_t buffer[], size_t size);
write_cb _write_cb = NULL;

void write(const uint8_t buffer[], size_t size)
{
  if (_write_cb)
  {
    _write_cb(buffer,size);
    
  }
}

void write16(uint16_t n)
{
  uint8_t _buf[2];
  _buf[0] = n;
  _buf[1] = n >> 8;
  write(_buf, 2);
}

void write32(uint32_t n)
{
  uint8_t _buf[4];
  _buf[0] = n;
  _buf[1] = n >> 8;
  _buf[2] = n >> 16;
  _buf[3] = n >> 24;
  write(_buf, 4);
}

void newPacket(uint32_t ts_sec, uint32_t ts_usec, uint32_t len, uint8_t *buf)
{
  const uint32_t orig_len = len;
  uint32_t incl_len = len;
  if (incl_len > snaplen)
  {
    incl_len = snaplen;
  }
  write32(ts_sec);
  write32(ts_usec);
  write32(incl_len);
  write32(orig_len);

  write(buf, incl_len);
}
void startCapture()
{
  write32(magic_number);
  write16(version_major);
  write16(version_minor);
  write32(thiszone);
  write32(sigfigs);
  write32(snaplen);
  write32(network);
}

#endif /* E653152C_730A_4ED6_9AA3_B1A780453B0A */
