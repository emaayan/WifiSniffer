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

static SemaphoreHandle_t addrOwnMacSem;
static addrFilter_t addrOwnMac = {{}, 0};

static SemaphoreHandle_t addr2Sem;
// static addrFilter_t addr2Filter = {{0x00, 0x0C, 0XCC}, 3};//{{}, 0}; //

static addrFilter_t addr2Filter = {{}, 0};

static SemaphoreHandle_t addr1Sem;
static addrFilter_t addr1Filter = {{}, 0};

void setAddrOwnMacFilter(addrFilter_t addrFilter)
{
    if (xSemaphoreTake(addrOwnMacSem, portMAX_DELAY))
    {
        addrOwnMac = addrFilter;
        if (!xSemaphoreGive(addrOwnMacSem))
        {
            ESP_LOGE(TAG, "Error releaseing addrOwnMac semaphore");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get addrOwnMac semaphore");
    }
}

void setAddr2Filter(addrFilter_t addrFilter)
{
    if (xSemaphoreTake(addr2Sem, portMAX_DELAY))
    {
        addr2Filter = addrFilter;
        if (!xSemaphoreGive(addr2Sem))
        {
            ESP_LOGE(TAG, "Error releaseing addr2 semaphore");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get addr2 semaphore");
    }
}
void setAddr1Filter(addrFilter_t addrFilter)
{
    if (xSemaphoreTake(addr1Sem, portMAX_DELAY))
    {
        addr1Filter = addrFilter;
        if (!xSemaphoreGive(addr1Sem))
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

static wifiPacketHandler_t _wifiPacketHandler;

wifiPacketHandler_t setPacketHander(wifiPacketHandler_t wifiPacketHandler)
{
    wifiPacketHandler_t wifip = _wifiPacketHandler;
    _wifiPacketHandler = wifiPacketHandler;
    return wifip;
}

bool filter_packet(wifi_mgmt_hdr *mgmt)
{
    // ESP_LOGI(TAG, "gettng Packet");
    return //! isAddrEquel(mgmt->bssid, addrOwnMac, addrOwnMacSem) &&
        isAddrEquel(mgmt->da, addr1Filter, addr1Sem) && isAddrEquel(mgmt->sa, addr2Filter, addr2Sem);
}

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buff;

    wifi_mgmt_hdr *mgmt = (wifi_mgmt_hdr *)pkt->payload;

    const bool filter = filter_packet(mgmt);
    if (filter)
    {
        ESP_LOGD(TAG, "ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d", MAC2STR(mgmt->sa), pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, mgmt->seqctl & 0xFF);
        if (_wifiPacketHandler)
        {
            _wifiPacketHandler(type, pkt, mgmt);
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

void set_filter_channel(uint8_t channel)
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
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler)); // callback function
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));                              // set 'true' the promiscuous mode
}

void wifi_sniffer_deinit()
{
    ESP_LOGI(TAG, "deinit Sniffer");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));      
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(NULL)); 
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

void wifi_sniffer_init_config()
{
    addr1Sem = xSemaphoreCreateMutex();
    addr2Sem = xSemaphoreCreateMutex();
    addrOwnMacSem = xSemaphoreCreateMutex();    
    filter_channel = 1;
}

#define SNIFFER_STACK_SIZE 10000
TaskHandle_t xHandle_sniff = NULL;
void sniffer_start()
{
    //wifi_sniffer_init();
    xTaskCreate(&sniffer_task, "sniffig_task", SNIFFER_STACK_SIZE, &snifConfig, 1, &xHandle_sniff);
}

void sniffer_stop()
{
    ESP_LOGI(TAG, "Stopping sniffer");
    wifi_sniffer_deinit();
     vTaskDelete(xHandle_sniff);
}
