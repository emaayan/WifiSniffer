#include <Arduino.h>
#include <Sniffer.h>
#include <esp_log.h>

#ifdef HAS_TFT
#include <DisplayLib.h>
#endif

// https://www.wireshark.org/docs/wsug_html_chunked/ChCustCommandLine.html
// https://www.wireshark.org/docs/wsug_html_chunked/ChCustCommandLine.html
// https://github.com/yannpom/esp32sniffer
// https://gist.github.com/ouoam/3462e73fcfc36d063ecd8f9a5ac77f12
typedef enum OUTPUT_TYPE
{
  TEXT,
  BINARY
} OUTPUT_TYPE_T;
OUTPUT_TYPE_T outType = BINARY;

void log(const char *fmt, ...)
{
  if (outType == TEXT)
  {
    const size_t sz = 250;
    char buffer[sz] = "";
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, argptr);
    va_end(argptr);
    printf("%s", buffer);
  }
}
// void logit(const char *fmt, va_list argptr)
// {

//   const size_t sz = 250;
//   char buffer[sz] = "";
//   vsnprintf(buffer, sizeof(buffer), fmt, argptr);
//   printf("%s", buffer);
// }

#include <TimeLib.h>

#ifdef HAS_TFT
void display(const wifi_promiscuous_pkt_t *ppkt, const wifi_ieee80211_mac_hdr_t *hdr, wifi_promiscuous_pkt_type_t type)
{
  print(0, 0, 2, TFT_WHITE, TFT_BLACK, 1, "PACKET=%s, CH=%02d, RSSI=%02d", wifi_sniffer_packet_type2str(type), ppkt->rx_ctrl.channel, ppkt->rx_ctrl.rssi);
  print(0, 20, 2, TFT_WHITE, TFT_BLACK, 1, "ADDR1=%02x:%02x:%02x:%02x:%02x:%02x", hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
  print(0, 40, 2, TFT_WHITE, TFT_BLACK, 1, "ADDR2=%02x:%02x:%02x:%02x:%02x:%02x", hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
  print(0, 60, 2, TFT_WHITE, TFT_BLACK, 1, "ADDR3=%02x%02x%02x%02x%02x%02x", hdr->addr3[0], hdr->addr3[1], hdr->addr3[2], hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]);
  print(0, 80, 2, TFT_WHITE, TFT_BLACK, 1, "ADDR4=%02x%02x%02x%02x%02x%02x", hdr->addr4[0], hdr->addr4[1], hdr->addr4[2], hdr->addr4[3], hdr->addr4[4], hdr->addr4[5]);
}
#endif
void clear()
{
  Serial.flush();
}
void display(const wifi_promiscuous_pkt_t *ppkt, const wifi_ieee80211_mac_hdr_t *hdr, wifi_promiscuous_pkt_type_t type)
{

  log(" PACKET=%s, CH=%02d, RSSI=%02d, SEQ=%u", wifi_sniffer_packet_type2str(type), ppkt->rx_ctrl.channel, ppkt->rx_ctrl.rssi, hdr->sequence_ctrl);
  log(" ADDR1=%02x:%02x:%02x:%02x:%02x:%02x", hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
  log(" ADDR2=%02x:%02x:%02x:%02x:%02x:%02x", hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
  log(" ADDR3=%02x%02x%02x%02x%02x%02x", hdr->addr3[0], hdr->addr3[1], hdr->addr3[2], hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]);
  log(" ADDR4=%02x%02x%02x%02x%02x%02x", hdr->addr4[0], hdr->addr4[1], hdr->addr4[2], hdr->addr4[3], hdr->addr4[4], hdr->addr4[5]);
  log("\n");
}

void writePacket(const wifi_promiscuous_pkt_t *ppkt, uint32_t packetLength)
{

  const uint32_t timestamp = now();                                         // current timestamp
  const uint32_t microseconds = (unsigned int)(micros() - millis() * 1000); // micro seconds offset (0 - 999)
  uint8_t *payload = const_cast<uint8_t *>(ppkt->payload);
  newPacket(timestamp, microseconds, packetLength, payload); // send packet via Serial
}

struct addrFilter
{
  uint8_t addr[6];
  int size;
};
addrFilter addr2Filter = {{0x00, 0x0C, 0XCC}, 3};
addrFilter addr1Filter = {{}, 0};
bool start = false;
bool isAddrEquel(const uint8_t addr[], const addrFilter filter)
{
  return filter.size == 0 || !memcmp(addr, filter.addr, filter.size);
}

void packet_cb(wifi_promiscuous_pkt_type_t type, const wifi_promiscuous_pkt_t *ppkt, const uint32_t packetLength)
{
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;

  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  const signed int rssi = ppkt->rx_ctrl.rssi;

  // if (hdr->addr2[0] == 0x00 && hdr->addr2[1] == 0x0C && hdr->addr2[2] == 0xCC)
  if (start && isAddrEquel(hdr->addr1, addr1Filter) && isAddrEquel(hdr->addr2, addr2Filter))
  {
    display(ppkt, hdr, type);
    if (outType == BINARY)
    {
      writePacket(ppkt, packetLength);
    }
  }
}

void signal_start()
{

  if (outType == BINARY)
  {
    // Serial.println("<<START>>");
    capture_start();
  }
  if (outType == TEXT)
  {
    log("Starting capture, addr2 filter %s", addr2Filter.addr);
  }
  start = true;
  clear();
}

// #define BUAD  115200// 921600
#ifndef BUAD
#define BUAD 250000 // 921600
#endif
void initSerial()
{

  Serial.begin(BUAD);
  // delay(2000);

   signal_start();
}
void writeSerial(const uint8_t buffer[], size_t size)
{
  Serial.write(buffer, size);
}

void setup(void)
{
  // esp_log_set_vprintf(&vprintf);
  initSerial();

#ifdef HAS_TFT
  initDisplay();
#endif
  _write_cb = &writeSerial;
  wifi_sniffer_init(&packet_cb);
}

// F1014096
// F2000CCC
// FC01
// S1
// S0

const size_t OP_CODE_SIZE = 2;
bool isOpCode(const char buff[], const char op[])
{
  return !strncmp(buff, op, OP_CODE_SIZE);
}

unsigned int selChannel = 0;
void filterChannel(int ch)
{
  printf("Change to %d", ch);
  selChannel = ch;
  wifi_sniffer_set_channel(selChannel);
}

int hex_to_decimal(char hexChar)
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
      log("Invalid hexadecimal character.\n");
      return -1;
    }
    else
    {
      byteArray[i] = (unsigned char)((highNibble << 4) | lowNibble);
    }
  }
  return i;
}

void printAddrFilter(addrFilter filter)
{
  int i;
  log("\n Addr Filter: ");
  for (i = 0; i < filter.size; i++)
  {
    if (i > 0)
      log(":");
    log("%02X", filter.addr[i]);
  }
  log("\n");
}

addrFilter filterAddr(char mac[], const size_t readBytes)
{
  addrFilter filter = {{}, 0};
  if (readBytes > 0)
  {
    if (readBytes % 2 == 0)
    {
      log("\nread filter %s", mac);
      size_t t = hex_to_byte_array(mac, readBytes, filter.addr);
      filter.size = t;
      printAddrFilter(filter);
    }
    else
    {
      return {{}, -1};
    }
  }
  return filter;
}

size_t read(char *buffer, size_t length)
{
  return Serial.readBytes(buffer, length);
}

long parseInt()
{
  return Serial.parseInt();
}
bool hasBytes()
{
  return Serial.available() >= OP_CODE_SIZE;
}
void read()
{

  if (hasBytes())
  {
    char op[OP_CODE_SIZE] = "";
    read(op, sizeof(op));
    log("Got %s ", op);
    // ESP_LOGI("ESP", "Got %s ", op);
    if (isOpCode(op, "S0"))
    {
      start = false;
    }

    if (isOpCode(op, "S1"))
    {      
      signal_start();
    }

    if (isOpCode(op, "OT"))
    {
      outType = TEXT;
      signal_start();
    }
    if (isOpCode(op, "OB"))
    {
      outType = BINARY;
      signal_start();
    }
    if (isOpCode(op, "FC"))
    {
      int ch = parseInt();
      filterChannel(ch);
    }

    if (isOpCode(op, "F1"))
    {
      char mac[12] = "";
      const size_t readBytes = read(mac, sizeof(mac));
      addrFilter filt = filterAddr(mac, readBytes);
      if (filt.size >= 0)
      {
        addr1Filter = filterAddr(mac, readBytes);
      }
    }

    if (isOpCode(op, "F2"))
    {
      char mac[12] = "";
      const size_t readBytes = read(mac, sizeof(mac));
      addrFilter filt = filterAddr(mac, readBytes);
      if (filt.size >= 0)
      {
        addr2Filter = filterAddr(mac, readBytes);
      }
    }
  }
}

void loop()
{
  delay(1000);

  while (1)
  {
    read();
    if (selChannel == 0)
    {
      uint8_t ch = changeChannel();

      // #ifdef DEBUG_CON
      //     printf("channel is %u\n", ch);
      // #endif
    }
    yield(); // We must yield() to stop a watchdog timeout.
  }
}
