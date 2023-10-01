#include "wifi_sniffer_nvs.h"

#include "esp_err.h"
#include "esp_log.h"
#include "string.h"

#define CONFIG_ESP_SNIFFER_FILTER_RSSI -96
#define MAC_SIZE 12
static const char *TAG = "SnifferLibNvs";
void sniffer_addrFilter_to_string(addrFilter_t addrFilter, char *stringbuf, size_t sz)
{
    if (addrFilter.size > 0)
    {
        nvs_bytes_to_hex(addrFilter.addr, addrFilter.size, ":", 1, stringbuf, sz);
    }
}
addrFilter_t wifi_sniffer_nvs_get_filter_mac(const char *key)
{
    addrFilter_t def_addrFilter = {.addr = {}, .size = 0};
    addrFilter_t addrFilter = {.addr = {}, .size = 0};
    ESP_ERROR_CHECK(nvs_get_array(SNIFFER_NS, key, addrFilter.addr, &addrFilter.size, def_addrFilter.addr, def_addrFilter.size));
    return addrFilter;
}
void wifi_sniffer_nvs_set_filter_mac(const char *key, addrFilter_t addrFilter)
{
    addrFilter_t prev_addrFilter = wifi_sniffer_nvs_get_filter_mac(key);
    if (addrFilter.size != prev_addrFilter.size || memcmp(addrFilter.addr, prev_addrFilter.addr, addrFilter.size))
    {
        ESP_ERROR_CHECK(nvs_set_array(SNIFFER_NS, key, addrFilter.addr, addrFilter.size));
        ///   ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_array(SNIFFER_NS, "filtermac", addrFilter.addr, addrFilter.size));
    }
    else
    {
        ESP_LOGD(TAG, "mac filter not changed in NVS %s ", "filtermac");
    }
}

rssi_t wifi_sniffer_nvs_get_filter_rssi()
{
    int32_t rssi = CONFIG_ESP_SNIFFER_FILTER_RSSI;
    nvs_get_num32i(SNIFFER_NS, "rssi", &rssi, CONFIG_ESP_SNIFFER_FILTER_RSSI);
    return rssi;
}

void wifi_sniffer_nvs_set_filter_rssi(const rssi_t rssi)
{
    const rssi_t prev_rssi = wifi_sniffer_nvs_get_filter_rssi();
    if (prev_rssi != rssi)
    {
        ESP_ERROR_CHECK(nvs_set_num32i(SNIFFER_NS, "rssi", rssi));
    }
    else
    {
        ESP_LOGD(TAG, "rssi not saved");
    }
}