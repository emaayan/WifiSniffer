#ifndef D66A8A39_265A_425A_BDAC_FE2EEC481D52
#define D66A8A39_265A_425A_BDAC_FE2EEC481D52

#include "stdint.h"
#include "nvs_lib.h"
#define SNIFFER_NS "snif3.conf"
typedef struct
{
    uint8_t addr[6];
    size_t size;
} addrFilter_t;

typedef int8_t rssi_t;
void sniffer_addrFilter_to_string(addrFilter_t addrFilter, char *stringbuf, size_t sz);
addrFilter_t wifi_sniffer_nvs_get_filter_mac(const char *key);
void wifi_sniffer_nvs_set_filter_mac(const char *key, addrFilter_t addrFilter);
rssi_t wifi_sniffer_nvs_get_filter_rssi();
void wifi_sniffer_nvs_set_filter_rssi(const rssi_t rssi);

#define ADDR_TO_STRING(ADDR, OUT) sniffer_addrFilter_to_string(addrFilter, OUT, sizeof(OUT));

#endif /* D66A8A39_265A_425A_BDAC_FE2EEC481D52 */
