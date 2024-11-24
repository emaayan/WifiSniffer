#include "wifi_sniffer.h"

#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"

#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_event_base.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"

#include <limits.h>
#include "wifi_sniffer_nvs.h"
#include "../../build/config/sdkconfig.h"

static const char *TAG = "WifiSniffer";

static SemaphoreHandle_t _filter_sem;
static addrFilter_t addr_own_mac = {{}, 0};
static addrFilter_t addr2_filter = {{}, 0};
static addrFilter_t addr3_filter = {{}, 0};

static esp_event_loop_handle_t sniffer_event_loop_handle = NULL;
ESP_EVENT_DEFINE_BASE(SNIFFER_EVENT);
static void sniffer_post_event(sniffer_event_types_t sniffer_event_types)
{
    if (sniffer_event_loop_handle)
    {
        esp_err_t err = esp_event_post_to(sniffer_event_loop_handle, SNIFFER_EVENT, sniffer_event_types, NULL, 0, pdMS_TO_TICKS(10));
        switch (err)
        {
        case ESP_ERR_TIMEOUT:
            ESP_LOGE(TAG, "timeout on event loop");
            break;
        default:
            ESP_ERROR_CHECK(err);
            break;
        }
    }
}

#define SNIFFER_EVENT_QUEUE_SIZE 20
static void sniffer_start_event_loop()
{
    static esp_event_loop_args_t sniffer_event_loop_task_config = {
        .queue_size = SNIFFER_EVENT_QUEUE_SIZE,
        .task_name = "sniffer_events_task",
        .task_priority = configMAX_PRIORITIES - 5,
        .task_stack_size = configMINIMAL_STACK_SIZE * 8,
        .task_core_id = tskNO_AFFINITY};

    ESP_ERROR_CHECK(esp_event_loop_create(&sniffer_event_loop_task_config, &sniffer_event_loop_handle));
}

static void sniffer_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    sniffer_event_handler_t event_handler = handler_args;
    event_handler(id, event_data);
}

void sniffer_register_event_handler(sniffer_event_handler_t event_handler)
{
    if (sniffer_event_loop_handle)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_register_with(sniffer_event_loop_handle, SNIFFER_EVENT, ESP_EVENT_ANY_ID, sniffer_event_handler, event_handler, NULL));
    }
}

static bool sniffer_set_filter(const char *key, addrFilter_t addrFilter, addrFilter_t *filter)
{

    if (xSemaphoreTake(_filter_sem, portMAX_DELAY))
    {
        char mac_filter[20] = "";
        ADDR_TO_STRING(addrFilter, mac_filter);

        *filter = addrFilter;
        ESP_LOGI(TAG, "Set filter %s, %s ", key, mac_filter);
        wifi_sniffer_nvs_set_filter_mac(key, addrFilter);
        if (!xSemaphoreGive(_filter_sem))
        {
            ESP_LOGE(TAG, "Error releaseing addrOwnMac semaphore");
        }
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get addrOwnMac semaphore");
    }
    return false;
}

void sniffer_set_own_mac_filter(addrFilter_t addrFilter)
{
    sniffer_set_filter("filter.own", addrFilter, &addr_own_mac);
}

#define ADDR2_KEY "filter.addr2"
void sniffer_set_addr2_filter(addrFilter_t addrFilter)
{

    if (sniffer_set_filter(ADDR2_KEY, addrFilter, &addr2_filter))
    {
        sniffer_post_event(SNIFFER_EVENT_FILTER_CHANGED);
    }
}
addrFilter_t sniffer_get_addr2_filter()
{
    return addr2_filter;
}

#define ADDR3_KEY "filter.addr3"
void sniffer_set_addr3_filter(addrFilter_t addrFilter)
{

    if (sniffer_set_filter(ADDR3_KEY, addrFilter, &addr3_filter))
    {
        sniffer_post_event(SNIFFER_EVENT_FILTER_CHANGED);
    }
}

bool isAddrEquel(const uint8_t addr[], const addrFilter_t filter)
{
    bool f = filter.size == 0 || !memcmp(addr, filter.addr, filter.size);
    return f;
}



bool filter_packet(wifi_mgmt_hdr_t *mgmt)
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

static QueueHandle_t _packet_queue;
static bool sniffer_create_queue(UBaseType_t txQSize)
{
    ESP_LOGI(TAG, "Sniffer Queue Size: %d", txQSize);
    QueueHandle_t queue = xQueueCreate(txQSize, sizeof(pcap_rec_t));
    if (!queue)
    {
        ESP_LOGE(TAG, "Failed to create queue");
        return false;
    }
    else
    {
		ESP_LOGI(TAG, "Created Queue");
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
        sniffer_post_event(SNIFFER_EVENT_QUEUE_FULL);
        ESP_LOGE(TAG, "Queue is Full");
        return false;
    }
}

unsigned rotate(unsigned short x, unsigned shift)
{
    return (x >> shift) |
           (x << ((sizeof(x) * CHAR_BIT - shift) %
                  (sizeof(x) * CHAR_BIT)));
}

seq_ctrl_t get_seq(int16_t seqctl)
{
    int16_t sq = rotate(seqctl, 4);
    seq_ctrl_t seq_ctrl = {.seq = sq & 0x0FFF, .frag = (sq & 0xF000) >> 12};
    return seq_ctrl;
}

int sniffer_to_string(wifi_promiscuous_pkt_t *pkt, char *buff, size_t sz)
{
    wifi_mgmt_hdr_t *mgmt = (wifi_mgmt_hdr_t *)pkt->payload;
    uint8_t *ta = mgmt->ta;
    int16_t seq_ctrl = mgmt->seqctl;
    seq_ctrl_t sq = get_seq(seq_ctrl);
    int16_t rssi = pkt->rx_ctrl.rssi;
    uint8_t channel = pkt->rx_ctrl.channel;
    int ret = snprintf(buff, sz, "ADDR2=" MACSTR ",seq=%d:%d , RSSI=%d ,Channel=%d ", MAC2STR(ta), sq.seq, sq.frag, rssi, channel);
    return ret;
    // ESP_LOGI(TAG, "ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d:%d", MAC2STR(mgmt->ta), pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, sq.seq, sq.frag);
}

#define SNIFFER_PAYLOAD_FCS_LEN 4
#define SNIFFER_RSSI_FILTER_DEFAULT -96
static rssi_t rssi_filter = SNIFFER_RSSI_FILTER_DEFAULT;

void sniffer_set_rssi_filter(rssi_t rssi)
{
    if (rssi == 0)
    {
        rssi = SNIFFER_RSSI_FILTER_DEFAULT;
    }
    rssi_filter = rssi;
    wifi_sniffer_nvs_set_filter_rssi(rssi);
    sniffer_post_event(SNIFFER_EVENT_FILTER_CHANGED);
}

rssi_t sniffer_get_rssi_filter()
{
    return rssi_filter;
}

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buff;

    if (type != WIFI_PKT_MISC && !pkt->rx_ctrl.rx_state)
    {
		const uint16_t fc= *(uint16_t*)pkt->payload;
				
        pkt->rx_ctrl.sig_len -= SNIFFER_PAYLOAD_FCS_LEN;
        const bool rssiFilter= (rssi_filter == 0 || (rssi_filter < 0 && pkt->rx_ctrl.rssi >= rssi_filter));
        bool isOwnAddress=false;
        bool macFilter=false; //=filter_packet(mgmt);
        if (xSemaphoreTake(_filter_sem, portMAX_DELAY))
    	{
        
	        if (!xSemaphoreGive(_filter_sem))
	        {
	            ESP_LOGE(TAG, "Failed to give filter semaphore");
	            macFilter= true;
	        }
	        else
	        {
				const uint8_t subtype = FC_SUBTYPE(fc);
			    const bool to_ds = FC_TO_DS(fc);
			    const bool from_ds = FC_FROM_DS(fc);
			    	    
		        
				switch (type) 
				{
					case WIFI_PKT_MGMT:
					{
						wifi_managment_hdr_t *wifi_managment_hdr =(wifi_managment_hdr_t *)pkt->payload;
					    isOwnAddress= 
					      isAddrEquel(wifi_managment_hdr->sa, addr_own_mac)  || isAddrEquel(wifi_managment_hdr->da, addr_own_mac) 
					   || isAddrEquel(wifi_managment_hdr->bssid, addr_own_mac) 
					   ;			
					  macFilter= isAddrEquel(wifi_managment_hdr->sa, addr2_filter)  || isAddrEquel(wifi_managment_hdr->da, addr2_filter)
					   ; 			  	
					}
					break;
					case WIFI_PKT_DATA:
					{
						if(!from_ds && !to_ds){					
							wifi_data_ibss_hdr_t *wifi_data_ibss_hdr=(wifi_data_ibss_hdr_t *)pkt->payload;
						    isOwnAddress=isAddrEquel(wifi_data_ibss_hdr->sa, addr_own_mac) || isAddrEquel(wifi_data_ibss_hdr->da, addr_own_mac);			   			   		
				        	macFilter=isAddrEquel(wifi_data_ibss_hdr->sa, addr2_filter);// || isAddrEquel(wifi_data_ibss_hdr->da, addr2_filter);			    			  				
						}else if(from_ds && !to_ds){
							wifi_data_from_ap_hdr_t *wifi_data_from_ap_hdr=(wifi_data_from_ap_hdr_t *)pkt->payload;
							isOwnAddress=isAddrEquel(wifi_data_from_ap_hdr->sa, addr_own_mac) || isAddrEquel(wifi_data_from_ap_hdr->da, addr_own_mac);			   			   		
				        	macFilter=isAddrEquel(wifi_data_from_ap_hdr->sa, addr2_filter);// || isAddrEquel(wifi_data_from_ap_hdr->da, addr2_filter);
						}else if(!from_ds && to_ds){
							wifi_data_to_ap_hdr_t *wifi_data_to_ap_hdr=(wifi_data_to_ap_hdr_t *)pkt->payload;
							isOwnAddress=isAddrEquel(wifi_data_to_ap_hdr->sa, addr_own_mac) || isAddrEquel(wifi_data_to_ap_hdr->da, addr_own_mac);			   			   		
				        	macFilter=isAddrEquel(wifi_data_to_ap_hdr->sa, addr2_filter);// || isAddrEquel(wifi_data_to_ap_hdr->da, addr2_filter);
						}else{
							wifi_data_wds_hdr_t *wifi_data_wds_hdr=(wifi_data_wds_hdr_t *)pkt->payload;				
							isOwnAddress=isAddrEquel(wifi_data_wds_hdr->ta, addr_own_mac) || isAddrEquel(wifi_data_wds_hdr->ra, addr_own_mac);			   			   		
					        macFilter=isAddrEquel(wifi_data_wds_hdr->ta, addr2_filter);// || isAddrEquel(wifi_data_wds_hdr->da, addr2_filter);
						}
						
					}
					break;
					case WIFI_PKT_CTRL:
					{
						switch (subtype) 
						{
							case WIFI_PKT_CTRL_SUBTYPE_BAR:
							{
								wifi_ctrl_ba_hdr_t *wifi_ctrl_ba_hdr=(wifi_ctrl_ba_hdr_t *)pkt->payload;
								isOwnAddress=isAddrEquel(wifi_ctrl_ba_hdr->ta ,addr_own_mac) || isAddrEquel(wifi_ctrl_ba_hdr->ra, addr_own_mac);			   			   		
								macFilter=isAddrEquel(wifi_ctrl_ba_hdr->ta, addr2_filter);// || isAddrEquel(wifi_ctrl_ba_hdr->ra, addr2_filter);
							}
							break;						
							case WIFI_PKT_CTRL_SUBTYPE_BA:
							{
								wifi_ctrl_ba_hdr_t *wifi_ctrl_ba_hdr=(wifi_ctrl_ba_hdr_t *)pkt->payload;
								isOwnAddress=isAddrEquel(wifi_ctrl_ba_hdr->ta ,addr_own_mac) || isAddrEquel(wifi_ctrl_ba_hdr->ra, addr_own_mac);			   			   		
								macFilter=isAddrEquel(wifi_ctrl_ba_hdr->ta, addr2_filter);// || isAddrEquel(wifi_ctrl_ba_hdr->ta, addr2_filter);	
							}
							break;
							case WIFI_PKT_CTRL_SUBTYPE_PS_POLL:
							{
								wifi_ctrl_ps_poll_hdr_t *wifi_ctrl_ps_poll_hdr=(wifi_ctrl_ps_poll_hdr_t *)pkt->payload;
								isOwnAddress=isAddrEquel(wifi_ctrl_ps_poll_hdr->ta, addr_own_mac);			   			   		
								macFilter=isAddrEquel(wifi_ctrl_ps_poll_hdr->ta, addr2_filter);
							}
							break;
							case WIFI_PKT_CTRL_SUBTYPE_RTS:
							{
								wifi_ctrl_rts_hdr_t *wifi_ctrl_rts_hdr=(wifi_ctrl_rts_hdr_t *)pkt->payload;
								isOwnAddress=isAddrEquel(wifi_ctrl_rts_hdr->ta ,addr_own_mac) || isAddrEquel(wifi_ctrl_rts_hdr->ra, addr_own_mac);			   			   		
								macFilter=isAddrEquel(wifi_ctrl_rts_hdr->ta, addr2_filter); //|| isAddrEquel(wifi_ctrl_rts_hdr->ra, addr2_filter);
							}
							break;
							case WIFI_PKT_CTRL_SUBTYPE_CTS:
							{
								wifi_ctrl_cts_hdr_t *wifi_ctrl_cts_hdr=(wifi_ctrl_cts_hdr_t *)pkt->payload;
								isOwnAddress=isAddrEquel(wifi_ctrl_cts_hdr->ra ,addr_own_mac);			   			   		
								macFilter=isAddrEquel(wifi_ctrl_cts_hdr->ra, addr2_filter);
							}
							break;
							case WIFI_PKT_CTRL_SUBTYPE_ACK:
							{
								wifi_ctrl_ack_hdr_t *wifi_ctrl_ack_hdr=(wifi_ctrl_ack_hdr_t *)pkt->payload;
								isOwnAddress=isAddrEquel(wifi_ctrl_ack_hdr->ra ,addr_own_mac);			   			   		
								macFilter=isAddrEquel(wifi_ctrl_ack_hdr->ra, addr2_filter);
							}
							break;
							default:
								break;					
						}
		
					}
					break;			
					default:
						break;
				}
				
			//	wifi_mgmt_hdr_t *mgmt = (wifi_mgmt_hdr_t *)pkt->payload;
			        
			}
		}
	    else
	    {
	        macFilter= true;
	    }
    
 		
		        
        const bool filter = rssiFilter && !isOwnAddress &&  macFilter; 
        if (filter)
        {
            // seq_ctrl_t sq = get_seq(mgmt->seqctl);
            // ESP_LOGI(TAG, "ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d:%d", MAC2STR(mgmt->ta), pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, sq.seq, sq.frag);

            pcap_rec_t pcap_rec = capture_create_pcap_record(pkt);
            sniffer_add_queue(&pcap_rec);
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
    ESP_LOGD(TAG, "Init Sniffer");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}

void wifi_sniffer_deinit()
{
    ESP_LOGD(TAG, "deinit Sniffer");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
}

sniffer_packet_t sniffer_to_packet_data(pcap_rec_t msg)
{
    wifi_mgmt_hdr_t *mgmt = (wifi_mgmt_hdr_t *)msg.buf;

    sniffer_packet_t sniffer_packet;
    sniffer_packet.sq = get_seq(mgmt->seqctl);
    snprintf(sniffer_packet.ta, sizeof(sniffer_packet.ta), MACSTR, MAC2STR(mgmt->ta));
    snprintf(sniffer_packet.addr3, sizeof(sniffer_packet.addr3), MACSTR, MAC2STR(mgmt->sa));
    sniffer_packet.fctl = mgmt->wifi_base_hdr.fctl;
    sniffer_packet.rssi = msg.r_tapdata.signal;
    sniffer_packet.channel = msg.r_tapdata.r_tapdata_channel.channel;
    return sniffer_packet;
    // ESP_LOGI(TAG, "GOT ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d:%d", MAC2STR(mgmt->ta), msg.r_tapdata.signal, msg.r_tapdata.r_tapdata_channel.channel, sq.seq, sq.frag);
}

void sniffer_task(void *pvParameter)
{
    //sniffer_config_t *cfg = pvParameter;
    // const int sleep_time = cfg->channelSwitchInterval;

    ESP_LOGI(TAG, "[SNIFFER] Starting sniffing mode...");
    wifi_sniffer_init();
    capture_start();
    sniffer_post_event(SNIFFER_EVENT_CAPTURE_STARTED);
    while (true)
    {

        pcap_rec_t msg;
        if (xQueueReceive(_packet_queue, &msg, pdMS_TO_TICKS(10)))
        {
            // wifi_mgmt_hdr_t *mgmt = (wifi_mgmt_hdr_t *)msg.buf;
            // seq_ctrl_t sq = get_seq(mgmt->seqctl);
            // ESP_LOGI(TAG, "GOT ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d:%d", MAC2STR(mgmt->ta), msg.r_tapdata.signal, msg.r_tapdata.r_tapdata_channel.channel, sq.seq, sq.frag);

            capture_on_send(msg);
        }
        // if (filter_channel == 0)
        // {
        //     switchChannels();
        // }
    }
}

static void sniffer_set_filter_packet_type(wifi_promiscuous_filter_t wifi_promiscuous_filter)
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&wifi_promiscuous_filter));
}

void sniffer_set_filter_data()
{
    wifi_promiscuous_filter_t wifi_promiscuous_filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA};
    sniffer_set_filter_packet_type(wifi_promiscuous_filter);
}
// no filter on packet types alone
void sniffer_set_no_filter()
{
    wifi_promiscuous_filter_t wifi_promiscuous_filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL};
    
    const wifi_promiscuous_filter_t wifi_promiscuous_ctrl_filter = {.filter_mask = WIFI_PROMIS_CTRL_FILTER_MASK_ALL};    
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_ctrl_filter(&wifi_promiscuous_ctrl_filter));
    
    sniffer_set_filter_packet_type(wifi_promiscuous_filter);
}

#define sniffer_queue_size CONFIG_SNIFFER_QUEUE_SIZE
void sniffer_init_config(addrFilter_t ownMac, sniffer_event_handler_t sniffer_event_handler)
{
    sniffer_create_queue(sniffer_queue_size);
    _filter_sem = xSemaphoreCreateMutex();
    filter_channel = 1;
    sniffer_set_own_mac_filter(ownMac);
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler));

    sniffer_start_event_loop();
    sniffer_register_event_handler(sniffer_event_handler);

    sniffer_post_event(SNIFFER_EVENT_IS_UP);

    rssi_t rssi = wifi_sniffer_nvs_get_filter_rssi();
    sniffer_set_rssi_filter(rssi);

    addrFilter_t addrFilter = wifi_sniffer_nvs_get_filter_mac(ADDR2_KEY);
    sniffer_set_addr2_filter(addrFilter);
}

TaskHandle_t xHandle_sniff = NULL;
void sniffer_start()
{
    // wifi_sniffer_init();    
    ESP_LOGI(TAG,"Sniffer Started");
    BaseType_t xRet=xTaskCreate(&sniffer_task, "sniffing_task", configMINIMAL_STACK_SIZE * 10, &snifConfig, configMAX_PRIORITIES - 24, &xHandle_sniff);
    if (xRet==errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
		ESP_LOGE(TAG,"Failed to create Task, Out of Memory");
	}    
}

void sniffer_stop()
{
    ESP_LOGI(TAG, "Stopping sniffer");
    wifi_sniffer_deinit();
    vTaskDelete(xHandle_sniff);
}
