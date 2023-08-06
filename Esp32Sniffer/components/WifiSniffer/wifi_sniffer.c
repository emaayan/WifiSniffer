#include <stdio.h>
#include "wifi_sniffer.h"
#include "../../build/config/sdkconfig.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"
static const char *TAG = "WifiSniffer";

static SemaphoreHandle_t addr_own_mac_sem;
static addrFilter_t addr_own_mac = {{}, 0};

static SemaphoreHandle_t add2_sem;
// static addrFilter_t addr2_filter = {{0x00, 0x0C, 0XCC}, 3};//{{}, 0}; //
 static addrFilter_t addr2_filter = {{}, 0};

static SemaphoreHandle_t addr3_sem;
static addrFilter_t addr3_filter = {{}, 0};

void sniffer_set_own_mac_filter(addrFilter_t addrFilter)
{
    if (xSemaphoreTake(addr_own_mac_sem, portMAX_DELAY))
    {
        addr_own_mac = addrFilter;
        if (!xSemaphoreGive(addr_own_mac_sem))
        {
            ESP_LOGE(TAG, "Error releaseing addrOwnMac semaphore");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get addrOwnMac semaphore");
    }
}

void sniffer_set_addr2_filter(addrFilter_t addrFilter)
{
    if (xSemaphoreTake(add2_sem, portMAX_DELAY))
    {
        addr2_filter = addrFilter;
        if (!xSemaphoreGive(add2_sem))
        {
            ESP_LOGE(TAG, "Error releaseing addr2 semaphore");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get addr2 semaphore");
    }
}
void sniffer_set_addr3_filter(addrFilter_t addrFilter)
{
    if (xSemaphoreTake(addr3_sem, portMAX_DELAY))
    {
        addr3_filter = addrFilter;
        if (!xSemaphoreGive(addr3_sem))
        {
            ESP_LOGE(TAG, "Error releaseing addr1 semaphore");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get addr1 semaphore");
    }
}

bool isAddrEquel(const uint8_t addr[], const addrFilter_t filter, SemaphoreHandle_t samHandle)
{
    if (xSemaphoreTake(samHandle, portMAX_DELAY))
    {
        bool f = filter.size == 0 || !memcmp(addr, filter.addr, filter.size);
        if (!xSemaphoreGive(samHandle))
        {
            ESP_LOGE(TAG, "Failed to give addr semaphore");
        }
        return f;
    }
    else
    {
        ESP_LOGE(TAG, "Failed filter with addr semaphore");
        return true;
    }
}

SemaphoreHandle_t _write_cb_sem;
static wifiPacketHandler_t _wifiPacketHandler;
// TODO: find out if we actually need semaphores for callbakc
wifiPacketHandler_t sniffer_set_packet_hander(wifiPacketHandler_t wifiPacketHandler)
{
    if (xSemaphoreTake(_write_cb_sem, portMAX_DELAY))
    {
        wifiPacketHandler_t wifip = _wifiPacketHandler;
        _wifiPacketHandler = wifiPacketHandler;
        xSemaphoreGive(_write_cb_sem);
        return wifip;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get Callback Semaphore");
        return NULL;
    }
}

bool filter_packet(wifi_mgmt_hdr *mgmt)
{
    // ESP_LOGI(TAG, "gettng Packet");
    return !(
       isAddrEquel(mgmt->sa, addr_own_mac, addr_own_mac_sem) 
    || isAddrEquel(mgmt->ra, addr_own_mac, addr_own_mac_sem) 
    || isAddrEquel(mgmt->ta, addr_own_mac, addr_own_mac_sem)
    ) 
    && (
        isAddrEquel(mgmt->sa, addr3_filter, addr3_sem) 
     && isAddrEquel(mgmt->ta, addr2_filter, add2_sem)
    );
}

wifi_mgmt_hdr *sniffer_get_wifi_mgmt_hdr(wifi_promiscuous_pkt_t *pkt)
{
    return (wifi_mgmt_hdr *)pkt->payload;
}

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buff;
    if (type == WIFI_PKT_MGMT)
    {
        pkt->rx_ctrl.sig_len -= 4; // sig_packetLength -= 4;
    }
    wifi_mgmt_hdr *mgmt = sniffer_get_wifi_mgmt_hdr(pkt);
    const bool filter = filter_packet(mgmt);
    if (filter)
    {
        ESP_LOGD(TAG, "ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d", MAC2STR(mgmt->ta), pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, mgmt->seqctl & 0xFF);
        if (_wifiPacketHandler)
        {
            _wifiPacketHandler(type, pkt);
        }
    }
}

typedef struct
{
    size_t chIndex;
    size_t chTotals;
    uint8_t channels[];
} channel_scan_range_t;

static channel_scan_range_t channel_scan_range = {
    .chIndex = 0,
    .chTotals = 3, // CHANGE THIS IF YOU CHANGE THE NUMBER OF CHANNELS
    .channels = {1, 6, 11},
};

uint8_t filter_channel = 0; // TODO: ADD LOCKS
uint8_t getNextChannel()
{
    channel_scan_range.chIndex = channel_scan_range.chIndex == channel_scan_range.chTotals - 1 ? 0 : channel_scan_range.chIndex + 1;
    const uint8_t channel = channel_scan_range.channels[channel_scan_range.chIndex];
    return channel;
}

#define WIFI_CHANNEL_SWITCH_INTERVAL (500)
#define START_CHANNEL 1
static sniffer_config_t snifConfig = {
    .startChannel = START_CHANNEL,
    .channelSwitchInterval = WIFI_CHANNEL_SWITCH_INTERVAL};

void wifi_sniffer_set_channel(uint8_t channel)
{
    ESP_LOGD(TAG, "Changing Channel to  %d", channel);
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
}

void sniffer_set_filter_channel(uint8_t channel)
{
    filter_channel = channel;
    if (filter_channel > 0)
    {
        ESP_LOGI(TAG, "Filtering channel to  %d", channel);
        wifi_sniffer_set_channel(filter_channel);
    }
    else
    {
        ESP_LOGI(TAG, "Clearing channel filter");
    }
}

uint8_t switchChannels()
{
    const uint8_t channel = getNextChannel();
    wifi_sniffer_set_channel(channel);
    return channel;
}

void wifi_sniffer_init()
{
    ESP_LOGI(TAG, "Init Sniffer");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}

void wifi_sniffer_deinit()
{
    ESP_LOGI(TAG, "deinit Sniffer");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
}

void sniffer_task(void *pvParameter)
{
    sniffer_config_t *cfg = pvParameter;
    const int sleep_time = cfg->channelSwitchInterval;

    ESP_LOGI(TAG, "[SNIFFER] Starting sniffing mode...");
    wifi_sniffer_init(*cfg);

    while (true)
    {

        vTaskDelay(sleep_time / portTICK_PERIOD_MS);
        // if (filter_channel == 0)
        // {
        //     switchChannels();
        // }
    }
}

void sniffer_init_config(wifiPacketHandler_t wifiPacketHandler, addrFilter_t ownMac)
{
    _write_cb_sem = xSemaphoreCreateMutex();
    addr3_sem = xSemaphoreCreateMutex();
    add2_sem = xSemaphoreCreateMutex();
    addr_own_mac_sem = xSemaphoreCreateMutex();
    filter_channel = 1;
    sniffer_set_packet_hander(wifiPacketHandler);
    sniffer_set_own_mac_filter(ownMac);
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler)); // callback function
}

#define SNIFFER_STACK_SIZE 10000
TaskHandle_t xHandle_sniff = NULL;
void sniffer_start()
{
    // wifi_sniffer_init();
    xTaskCreate(&sniffer_task, "sniffig_task", SNIFFER_STACK_SIZE, &snifConfig, 1, &xHandle_sniff);
}

void sniffer_stop()
{
    ESP_LOGI(TAG, "Stopping sniffer");
    wifi_sniffer_deinit();
    vTaskDelete(xHandle_sniff);
}
