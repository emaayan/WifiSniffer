#include <wifi_lib.h>

#include "freertos/event_groups.h"
#include <stdint.h>

static const char *TAG = "WifiLib";

static esp_event_loop_handle_t wifi_lib_event_loop_handle = NULL;
ESP_EVENT_DEFINE_BASE(WIFI_LIB_EVENT);
static void wifi_lib_post_event(wifi_lib_event_types_t wifi_lib_event_types)
{
    if (wifi_lib_event_loop_handle)
    {
        esp_err_t err = esp_event_post_to(wifi_lib_event_loop_handle, WIFI_LIB_EVENT, wifi_lib_event_types, NULL, 0, pdMS_TO_TICKS(10));
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

#define WIFI_LIB_EVENT_QUEUE_SIZE 20
static void wifi_lib_start_event_loop()
{
    static esp_event_loop_args_t wifi_lib_event_loop_task_config = {
        .queue_size = WIFI_LIB_EVENT_QUEUE_SIZE,
        .task_name = "wifi_lib_events_task",
        .task_priority = configMAX_PRIORITIES - 5,
        .task_stack_size = configMINIMAL_STACK_SIZE * 5,
        .task_core_id = tskNO_AFFINITY};

    ESP_ERROR_CHECK(esp_event_loop_create(&wifi_lib_event_loop_task_config, &wifi_lib_event_loop_handle));
}

static void wifi_lib_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    wifi_lib_event_handler_t event_handler = handler_args;
    event_handler(id, event_data);
}

void wifi_lib_register_event_handler(wifi_lib_event_handler_t event_handler)
{
    if (wifi_lib_event_loop_handle)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_register_with(wifi_lib_event_loop_handle, WIFI_LIB_EVENT, ESP_EVENT_ANY_ID, wifi_lib_event_handler, event_handler, NULL));
    }
    else
    {
    }
}

static EventGroupHandle_t s_wifi_event_group;

static esp_netif_t *_esp_netif = NULL;
static void set_net_if(esp_netif_t *esp_netif)
{
    _esp_netif = esp_netif;
}
static esp_netif_t *get_net_if()
{
    return _esp_netif;
}

static void destroy_net_if()
{
    esp_netif_t *esp_netif = get_net_if();
    if (esp_netif)
    {
        ESP_LOGI(TAG, "Destorying netif");
        esp_netif_destroy_default_wifi(esp_netif);
        set_net_if(NULL);
    }
}
static esp_err_t wifi_set_dns_server(esp_netif_t *netif, esp_ip4_addr_t addr, esp_netif_dns_type_t type)
{

    if (wifi_is_valid_ip(addr))
    {
        esp_netif_dns_info_t dns = {.ip.u_addr.ip4 = addr, dns.ip.type = IPADDR_TYPE_V4};
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
        ESP_LOGI(TAG, "Set DNS " IPSTR, IP2STR(&dns.ip.u_addr.ip4));
    }
    return ESP_OK;
}

static void wifi_conf_static_ip(esp_netif_ip_info_t ip, dns_servers_info_t dns_servers_info)
{
    esp_netif_t *netif = get_net_if();
    if (netif)
    {
        wifi_lib_mode_t wifi_lib_mode = wifi_nvs_get_mode();
        if (wifi_lib_mode == WIFI_LIB_MODE_AP)
        {
            if (wifi_is_valid_ip_info(ip))
            {
                esp_err_t err = esp_netif_dhcpc_stop(netif);
                if (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)
                {
                    ESP_ERROR_CHECK(err);
                }

                ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
                ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip));
                ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));

                char ip_addr[ADDRESS_SZ] = "";
                snprintf(ip_addr, sizeof(ip_addr), IPSTR, IP2STR((&ip.ip)));

                char netmask_addr[ADDRESS_SZ] = "";
                snprintf(netmask_addr, sizeof(netmask_addr), IPSTR, IP2STR((&ip.netmask)));

                char gw_addr[ADDRESS_SZ] = "";
                snprintf(gw_addr, sizeof(gw_addr), IPSTR, IP2STR((&ip.gw)));
                wifi_nvs_set_static_ip_info(ip_addr, netmask_addr, gw_addr);
                wifi_lib_post_event(WIFI_LIB_GOT_IP);
                ESP_LOGI(TAG, "Success to set static ip: %s, netmask: %s , gw: %s", ip_addr, netmask_addr, gw_addr); // check this still works
                wifi_set_dns_server(netif, dns_servers_info.primary_dns, ESP_NETIF_DNS_MAIN);
                wifi_set_dns_server(netif, dns_servers_info.secondery_dns, ESP_NETIF_DNS_BACKUP);
            }
            else
            {
                ESP_LOGE(TAG, "Cannot configure Static IP");
            }
        }
        else
        {
            ESP_LOGI(TAG, "Static IP can only be configured in AP mode");
        }
    }
}

void wifi_conf_set_static_ip(esp_netif_ip_info_t ip)
{
    dns_servers_info_t dns = {.primary_dns.addr = IPADDR_NONE, .secondery_dns.addr = IPADDR_NONE};
    wifi_conf_static_ip(ip, dns);
}

static int s_retry_num = 0;

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{  int8_t power = 0;
    switch (event_id)
    {

    case IP_EVENT_STA_GOT_IP:
    {
        s_retry_num = 0;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        wifi_lib_post_event(WIFI_LIB_GOT_IP);
        ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_ERROR_CHECK(esp_wifi_get_max_tx_power(&power));
    	ESP_LOGI(TAG, "Max TX Power: %d", power);
        break;
    }
    case IP_EVENT_AP_STAIPASSIGNED:
    {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
        ESP_LOGI(TAG, "IP assigned :" IPSTR, IP2STR(&event->ip));
        ESP_ERROR_CHECK(esp_wifi_get_max_tx_power(&power));
    	ESP_LOGI(TAG, "Max TX Power: %d", power);
        break;
    }
    case IP_EVENT_STA_LOST_IP:
    {
        ESP_LOGW(TAG, "station lost IP and the IP is reset to 0");
        break;
    }
    default:
        ESP_LOGI(TAG, "IP Event %" PRIu32, event_id);
        break;
    }
}
// https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32/api-guides/wifi.html?highlight=setsockopt#esp32-wi-fi-event-description

#define WIFI_EVENT_AP_START_BIT BIT0
#define WIFI_EVENT_STA_CONNECTED_BIT BIT1
#define WIFI_EVENT_STA_DISCONNECTED_BIT BIT2
#define WIFI_EVENT_STA_START_BIT BIT3
#define WIFI_EVENT_AP_STOP_BIT BIT4
#define WIFI_EVENT_STA_STOP_BIT BIT5
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
    {
        ESP_LOGI(TAG, "STA Started");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_START_BIT);
        break;
    }
    case WIFI_EVENT_STA_STOP:
    {
        ESP_LOGI(TAG, "STA Stopped");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_STOP_BIT);
        break;
    }
    case WIFI_EVENT_AP_START:
    {
        ESP_LOGI(TAG, "Started soft AP");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_AP_START_BIT);
        wifi_lib_post_event(WIFI_LIB_HAS_SSID);
        break;
    }
    case WIFI_EVENT_AP_STOP:
    {
        ESP_LOGI(TAG, "Stopped Soft AP");
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_AP_STOP_BIT);
        break;
    }
    case WIFI_EVENT_AP_STACONNECTED:
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        wifi_lib_post_event(WIFI_LIB_LEFT_SSID);
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_STA_CONNECTED:
    {
        wifi_event_sta_connected_t *wifi_event_sta_connected = (wifi_event_sta_connected_t *)event_data;
        ESP_LOGI(TAG, "Connected STA to %s on channel %d", wifi_event_sta_connected->ssid, wifi_event_sta_connected->channel);
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_CONNECTED_BIT);
        wifi_lib_post_event(WIFI_LIB_JOINED_SSID);
        break;
    }
    case WIFI_EVENT_STA_DISCONNECTED:
    {
        wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGI(TAG, "Disconnected From %s with rssi %d ,due to: %d", wifi_event_sta_disconnected->ssid, wifi_event_sta_disconnected->rssi, wifi_event_sta_disconnected->reason);
        // esp_wifi_connect();
        // s_retry_num++;
        xEventGroupSetBits(s_wifi_event_group, WIFI_EVENT_STA_DISCONNECTED_BIT);
        wifi_lib_post_event(WIFI_LIB_LEFT_SSID);
        break;
    }
    case WIFI_EVENT_STA_BEACON_TIMEOUT:
    {
        ESP_LOGW(TAG, "STA Beacon timeout");
        break;
    }
    default:
        ESP_LOGI(TAG, "STA Event %" PRIu32, event_id);
        break;
    }
}

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html
// https://esp32tutorials.com/esp32-static-fixed-ip-address-esp-idf/
// https://github.com/espressif/esp-idf/blob/master/examples/wifi/softap_sta/main/softap_sta.c

#define CONFIG_MAX_STA_CONN 2 // CONFIG_MAX_STA_CONN_TO_AP

static int wifi_generate_ssid(uint8_t *ssid_prefix, char *gen_ssid, size_t gen_ssid_sz)
{
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, mac));
    int sz = snprintf(gen_ssid, gen_ssid_sz, "%s_%02X_%02X_%02X", ssid_prefix, mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "SSID will be %s:  ", gen_ssid);
    return sz;
}

static esp_err_t wifi_set_conf(wifi_mode_t wifi_mode,wifi_interface_t interface, wifi_config_t conf,wifi_config_t conf_def)
{
  wifi_country_t country = 
  {
      .cc = CONFIG_DEF_CC,
      .schan = CONFIG_DEF_CC_SCHAN,
      .nchan = CONFIG_DEF_CC_NCHAN,
      .max_tx_power = CONFIG_DEF_CC_MAX_TX,
      .policy = WIFI_COUNTRY_POLICY_AUTO,
  };
  ESP_ERROR_CHECK(esp_wifi_set_country(&country));

  ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode));
  esp_err_t esp_wifi_set_config_err = esp_wifi_set_config(interface, &conf);
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_config_err);
  if (esp_wifi_set_config_err != ESP_OK) 
  {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_config(interface, &conf_def));
  }
  return esp_wifi_set_config_err;
}

static wifi_config_t wifi_cfg_ap_def = {
    .ap = {.ssid = CONFIG_DEF_AP_SSID, // in case of an error it won't generate
                                       // the mac's prefix
           .ssid_len = sizeof(CONFIG_DEF_AP_SSID),
           .password = CONFIG_DEF_AP_SSID_PW,
           .channel = CONFIG_SOFTAP_WIFI_CHANNEL,
           .max_connection = CONFIG_MAX_STA_CONN,
           .authmode = WIFI_AUTH_WPA_WPA2_PSK,
           .pmf_cfg = {
               .required = false,
           }}};


static esp_netif_t *wifi_soft_ap(ssid_cfg_t ssid_cfg, uint8_t channel)
{

    wifi_config_t wifi_cfg = {.ap = {
                                  .channel = channel,
                                  .max_connection = CONFIG_MAX_STA_CONN,
                                  .authmode = ssid_cfg.pass_sz == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK,
                                  .pmf_cfg = {
                                      .required = false,
                                  }}};

    char gen_ssid[SSID_SZ] = "";
    int sz = wifi_generate_ssid(ssid_cfg.ssid, gen_ssid, sizeof(gen_ssid));

    memcpy(wifi_cfg.ap.ssid, gen_ssid, sz);
    wifi_cfg.ap.ssid_len = sz;
    memcpy(wifi_cfg.ap.password, ssid_cfg.password, ssid_cfg.pass_sz);
    wifi_set_conf(WIFI_MODE_AP, WIFI_IF_AP, wifi_cfg, wifi_cfg_ap_def);

    ESP_LOGI(TAG, "Creating ap netif");
    esp_netif_t *netif = esp_netif_create_default_wifi_ap();
    return netif;
}

static wifi_config_t cfg_sta_def ={.sta =  {
	  .ssid=CONFIG_DEF_STA_SSID,	  
      .password=CONFIG_DEF_STA_SSID_PW,	  
      .scan_method = WIFI_ALL_CHANNEL_SCAN,
      .threshold.authmode = WIFI_AUTH_WPA2_PSK,
  }};
  

  
static esp_netif_t *wifi_set_sta(ssid_cfg_t ssid_cfg)
{
    wifi_sta_config_t staConf = {
        .scan_method = WIFI_ALL_CHANNEL_SCAN,
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    };
    memcpy(staConf.ssid, ssid_cfg.ssid, ssid_cfg.ssid_sz);
    memcpy(staConf.password, ssid_cfg.password, ssid_cfg.pass_sz);
    wifi_config_t cfg = {.sta = staConf};

    wifi_set_conf(WIFI_MODE_STA, WIFI_IF_STA, cfg,cfg_sta_def);

    ESP_LOGD(TAG, "Attempting to join %s:  ", staConf.ssid);
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    return netif;
}

void wifi_init(wifi_lib_event_handler_t wifi_lib_event_handler)
{
    ESP_LOGI(TAG, "Init Wifi");

    wifi_lib_start_event_loop();
    wifi_lib_register_event_handler(wifi_lib_event_handler);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, &instance_got_ip));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // allocate resource for WiFi driver
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));//for the timer in capture record, make it more accurate 
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

void wifi_get_mac(uint8_t mac[])
{
    esp_netif_t *esp_netif = get_net_if();
    if (esp_netif)
    {
        ESP_ERROR_CHECK(esp_netif_get_mac(get_net_if(), mac));
    }
}

void wifi_get_ip(char msg[], size_t sz)
{
    esp_netif_ip_info_t ipInfo;
    esp_netif_t *esp_netif = get_net_if();
    if (esp_netif)
    {
        ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif, &ipInfo));
        snprintf(msg, sz, "" IPSTR, IP2STR(&ipInfo.ip));
    }
}

static wifi_lib_cfg_t wifi_lib_get_config(esp_netif_t *esp_netif)
{
    wifi_lib_cfg_t wifi_lib_cfg;
    if (esp_netif)
    {
        wifi_mode_t wifi_mode;
        ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));
        wifi_config_t wifi_config;
        switch (wifi_mode)
        {
        case WIFI_MODE_AP:
        {
            ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_AP, &wifi_config));
            wifi_lib_cfg.wifi_lib_mode = WIFI_LIB_MODE_AP;
            memcpy(wifi_lib_cfg.ssid_name, wifi_config.ap.ssid, sizeof(wifi_lib_cfg.ssid_name));
        }
        break;
        case WIFI_MODE_STA:
        {
            ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
            wifi_lib_cfg.wifi_lib_mode = WIFI_LIB_MODE_STA;
            wifi_ap_record_t wifi_ap_record;
            esp_err_t err = esp_wifi_sta_get_ap_info(&wifi_ap_record);
            if (err != ESP_OK)
            {
                memset(wifi_lib_cfg.ssid_name, ' ', sizeof(wifi_lib_cfg.ssid_name));
            }
            else
            {
                memcpy(wifi_lib_cfg.ssid_name, wifi_ap_record.ssid, sizeof(wifi_lib_cfg.ssid_name));
            }
        }
        break;
        default:
            ESP_LOGE(TAG, "Unexepcted Mode");
            break;
        }
        uint8_t channel = 0;
        wifi_second_chan_t wifi_second_chan = 0;
        esp_err_t err = esp_wifi_get_channel(&channel, &wifi_second_chan);
        if (err != ESP_ERR_WIFI_STOP_STATE)
        {
            ESP_ERROR_CHECK(err);
        }
        wifi_lib_cfg.channel = channel;

        esp_netif_ip_info_t ip_info;
        ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif, &ip_info));
        wifi_lib_cfg.ip = ip_info.ip;

        ESP_ERROR_CHECK(esp_netif_get_mac(esp_netif, wifi_lib_cfg.mac));
    }
    else
    {
        ESP_LOGE(TAG, "net if is not up");
        wifi_lib_cfg.wifi_lib_mode = WIFI_LIB_MODE_NONE;
    }
    return wifi_lib_cfg;
}
wifi_lib_cfg_t wifi_lib_get_curr_config()
{
    esp_netif_t *net_if = get_net_if();
    return wifi_lib_get_config(net_if);
}

void wifi_ap(const ssid_cfg_t ssid_cfg, uint8_t channel, esp_netif_ip_info_t ip, dns_servers_info_t dns_servers_info)
{
    ESP_LOGI(TAG, "Setting SoftAP");
    ESP_ERROR_CHECK(esp_wifi_stop());
    destroy_net_if();

    esp_netif_t *esp_netif = wifi_soft_ap(ssid_cfg, channel);
    set_net_if(esp_netif);
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_nvs_set_mode(WIFI_LIB_MODE_AP);
    wifi_nvs_set_ap_channel(channel);
    wifi_nvs_set_ap_ssid((char*)ssid_cfg.ssid, ssid_cfg.ssid_sz);
    wifi_nvs_set_ap_ssid_pw((char*)ssid_cfg.password, ssid_cfg.pass_sz);
    
    ESP_LOGI(TAG, "Waiting for AP Mode");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_EVENT_AP_START_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_EVENT_AP_START_BIT)
    {
        wifi_conf_static_ip(ip, dns_servers_info);
        char msg[50] = "";
        wifi_get_ip(msg, sizeof(msg));
        ESP_LOGI(TAG, "IP: %s", msg);
	    int8_t power = 0;
	    ESP_ERROR_CHECK(esp_wifi_get_max_tx_power(&power));
	    ESP_LOGI(TAG, "Max TX Power: %d", power);
        // wifi_lib_cfg_t wifi_lib_cfg = wifi_lib_get_config(esp_netif);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT %" PRIu32, bits);
    }
}

void wifi_sta(const ssid_cfg_t ssid_cfg_sta)
{
    ESP_LOGI(TAG, "Setting STA mode");
    ESP_ERROR_CHECK(esp_wifi_stop());
    destroy_net_if();
    esp_netif_t *esp_netif = wifi_set_sta(ssid_cfg_sta);
    set_net_if(esp_netif);

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for ESP to join");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_EVENT_STA_START_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_EVENT_STA_START_BIT)
    {
        ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_netif)); // MUST NOT HAVE DHCP SERVER WHEN JOINING A NETWORK

        ESP_ERROR_CHECK(esp_wifi_connect());
        wifi_nvs_set_mode(WIFI_LIB_MODE_STA);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT %" PRIu32, bits);
    }
}

void wifi_set_mode(wifi_lib_mode_t wifi_lib_mode)
{
    switch (wifi_lib_mode)
    {
    case WIFI_LIB_MODE_AP:
    {
        ESP_LOGI(TAG, "Setting mode AP");
        ssid_cfg_t ssid_cfg = wifi_nvs_get_ssid_ap_cfg();
        uint8_t channel = wifi_nvs_get_ap_channel();
        esp_netif_ip_info_t esp_netif_ip_info = wifi_nvs_get_static_ip_info();
        dns_servers_info_t dns_servers_info = wifi_nvs_get_dns_servers();
        wifi_ap(ssid_cfg, channel, esp_netif_ip_info, dns_servers_info);
        break;
    }
    case WIFI_LIB_MODE_STA:
    {
        ESP_LOGI(TAG, "Setting mode STA");
        ssid_cfg_t ssid_cfg = wifi_nvs_get_ssid_sta_cfg();
        wifi_sta(ssid_cfg);
        break;
    }
    case WIFI_LIB_MODE_NONE:
        ESP_LOGI(TAG, "Setting mode NONE");
        break;
    default:
        ESP_LOGE(TAG, "Unsupported Mode");
        break;
    }

    char msg[50] = "";
    wifi_get_ip(msg, sizeof(msg));
}
