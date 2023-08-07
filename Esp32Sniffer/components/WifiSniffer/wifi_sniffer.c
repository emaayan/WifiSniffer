#include <stdio.h>

#include "wifi_sniffer.h"
#include "capture_lib.h"

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

#include "../../build/config/sdkconfig.h"

static const char *TAG = "WifiSniffer";

static SemaphoreHandle_t _filter_sem;
static addrFilter_t addr_own_mac = {{}, 0};
static addrFilter_t addr2_filter = {{}, 0};
static addrFilter_t addr3_filter = {{}, 0};

void sniffer_set_filter(addrFilter_t addrFilter, addrFilter_t *filter)
{
    if (xSemaphoreTake(_filter_sem, portMAX_DELAY))
    {
        *filter = addrFilter;
        if (!xSemaphoreGive(_filter_sem))
        {
            ESP_LOGE(TAG, "Error releaseing addrOwnMac semaphore");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get addrOwnMac semaphore");
    }
}

void sniffer_set_own_mac_filter(addrFilter_t addrFilter)
{
    sniffer_set_filter(addrFilter, &addr_own_mac);
}

void sniffer_set_addr2_filter(addrFilter_t addrFilter)
{
    sniffer_set_filter(addrFilter, &addr2_filter);
}
void sniffer_set_addr3_filter(addrFilter_t addrFilter)
{
    sniffer_set_filter(addrFilter, &addr3_filter);
}

bool isAddrEquel(const uint8_t addr[], const addrFilter_t filter)
{
    bool f = filter.size == 0 || !memcmp(addr, filter.addr, filter.size);
    return f;
}

bool filter_packet(wifi_mgmt_hdr *mgmt)
{
    if (xSemaphoreTake(_filter_sem, portMAX_DELAY))
    {
        bool f = !(
                     isAddrEquel(mgmt->sa, addr_own_mac) || isAddrEquel(mgmt->ra, addr_own_mac) || isAddrEquel(mgmt->ta, addr_own_mac)) &&
                 (isAddrEquel(mgmt->sa, addr3_filter) && isAddrEquel(mgmt->ta, addr2_filter));
        if (!xSemaphoreGive(_filter_sem))
        {
            ESP_LOGE(TAG, "Failed to give filter semaphore");
            return f;
        }
        else
        {
            return f;
        }
    }
    else
    {
        return true;
    }
}

wifi_mgmt_hdr *sniffer_get_wifi_mgmt_hdr(wifi_promiscuous_pkt_t *pkt)
{
    return (wifi_mgmt_hdr *)pkt->payload;
}

static QueueHandle_t _packet_queue;
static bool sniffer_create_queue(UBaseType_t txQSize)
{
    QueueHandle_t queue = xQueueCreate(txQSize, sizeof(pcap_rec_t));
    if (!queue)
    {
        ESP_LOGE(TAG, "Failed to create queue");
        return false;
    }
    else
    {
        _packet_queue = queue;
        return true;
    }
}

bool sniffer_add_queue(pcap_rec_t *msg)
{
    // ESP_LOGI(TAG, "Sending pcap Message %"PRIu32,msg->pcaprec_hdr.incl_len);
    if (xQueueSend(_packet_queue, msg, (TickType_t)0))
    {
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "Queue is Full %" PRIu32, msg->pcap_rec_hdr.incl_len);
        return false;
    }
}

static void write_packet(const wifi_promiscuous_pkt_type_t type, wifi_promiscuous_pkt_t *pkt)
{
    uint32_t sig_packetLength = pkt->rx_ctrl.sig_len;
    uint8_t *payload = pkt->payload;
    pcap_rec_t pcap_rec = capture_create_packet(sig_packetLength, payload);
    sniffer_add_queue(&pcap_rec);
}

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buff;
    if (type == WIFI_PKT_MGMT)
    {
        pkt->rx_ctrl.sig_len -= 4; // due to bug in esp-idf
    }
    wifi_mgmt_hdr *mgmt = sniffer_get_wifi_mgmt_hdr(pkt);
    const bool filter = filter_packet(mgmt);
    if (filter)
    {
        ESP_LOGD(TAG, "ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d", MAC2STR(mgmt->ta), pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, mgmt->seqctl & 0xFF);

        write_packet(type, pkt);
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
    capture_start();
    while (true)
    {

        pcap_rec_t msg;
        if (xQueueReceive(_packet_queue, &msg, (TickType_t)20))
        {
            capture_on_send(msg);
        }
        // if (filter_channel == 0)
        // {
        //     switchChannels();
        // }
    }
}
#define queue_size 20
void sniffer_init_config(addrFilter_t ownMac)
{
    sniffer_create_queue(queue_size);
    _filter_sem = xSemaphoreCreateMutex();
    filter_channel = 1;
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
