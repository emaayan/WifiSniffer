#include <stdio.h>

#include "OledDisplayUtils.h"
#include "wifi_lib.h"
#include "wifi_sniffer.h"
#include "tcp_server.h"
#include "SerialLib.h"
#include "capture_lib.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "../build/config/sdkconfig.h"
static const char *TAG = "WifiSnifferMain";
// https://github.com/espressif/esp-idf/blob/master/examples/common_components/protocol_examples_common/
//  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html
//  https://www.youtube.com/watch?v=STGK2nT8S9Q
typedef struct
{
    u_int16_t type;
    int8_t *data; //[MAX_MSG_SIZE];
    size_t size;
} message_t;

#define WRITE_BUFF 512
int serial_write(uart_port_t port, const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    char buff[WRITE_BUFF] = "";
    int r = serial_v_write(port, buff, sizeof(buff), fmt, argptr);
    va_end(argptr);
    return r;
}

QueueHandle_t txQueue;
QueueHandle_t createMessageQueue(size_t size)
{
    return xQueueCreate(size, sizeof(message_t));
}

bool sendMessage(message_t *msg)
{
    ESP_LOGI("QueueSend", "Sending Message %d, %s ", msg->size, msg->data);
    if (xQueueSend(txQueue, msg, (TickType_t)0))
    {

        return true;
    }
    else
    {
        return false;
    }
}

bool receiveMessage(message_t *msg)
{

    if (xQueueReceive(txQueue, msg, (TickType_t)0))
    {
        ESP_LOGI("QueueReceive", "Getting Message %d, %s ", msg->size, msg->data);
        return true;
    }
    else
    {
        return false;
    }
}

void copyFromSerial2ToSerialon1(serial_messsage_t serMsg) // sample on how to send messsage
{

    const size_t sz = serMsg.size;
    int8_t *data = serMsg.data;

    message_t msg;
    msg.data = malloc((sizeof(u_int8_t) * sz) + 1); //+1 for null chachater
    msg.size = sz;
    memcpy(msg.data, data, sz);
    msg.data[msg.size] = 0;

    sendMessage(&msg);
}

const size_t OP_CODE_SIZE = 2;
bool isOpCode(const char buff[], const char op[])
{
    return !strncmp(buff, op, OP_CODE_SIZE);
}
bool start = false;
typedef enum OUTPUT_TYPE
{
    TEXT,
    BINARY
} OUTPUT_TYPE_T;
OUTPUT_TYPE_T outType = TEXT;

void writePacket(const wifi_promiscuous_pkt_type_t type, wifi_promiscuous_pkt_t *pkt, wifi_mgmt_hdr *mgmt)
{
    if (start)
    {
        uint32_t sig_packetLength = pkt->rx_ctrl.sig_len;
        if (type == WIFI_PKT_MGMT)
        {
            sig_packetLength -= 4;
        }
        uint8_t *payload = pkt->payload;

        // send_msg("ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d", MAC2STR(mgmt->sa), pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, mgmt->seqctl & 0xFF);
        ESP_LOGI(TAG, "ADDR2=" MACSTR " , RSSI=%d ,Channel=%d ,seq=%d", MAC2STR(mgmt->sa), pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, mgmt->seqctl & 0xFF);
        addPacket(sig_packetLength, payload); // send packet via Serial
    }
}

void signalStart()
{
    start = true;
    startCapture();
    sniffer_start();
}

void signalStop()
{
    start = false;
    sniffer_stop();
}

int _sock = 0;
void on_socket_accept_handler(const int sock, struct sockaddr_in *so_in)
{
    disconnect_socket(_sock);
    _sock = sock;
    signalStart();
}

int on_write(void *buffer, size_t size)
{
    const size_t sz = size;
    int8_t *data = buffer;
    int wrote = 0; // write_serial_0(data, sz);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, data, size, ESP_LOG_DEBUG);
    if (_sock)
    {
        if (!onSend(_sock, buffer, sz))
        {
            _sock = 0;
            sniffer_stop();
        }
    }
    else
    {
        // ESP_LOGE(TAG, "No Socket");
    }

    return wrote;

    // message_t msg;
    // msg.data = malloc((sizeof(u_int8_t) * sz) + 1); //+1 for null chachater
    // msg.size = sz;
    // memcpy(msg.data, data, sz);
    // msg.data[msg.size] = 0;

    // sendMessage(&msg);
}

void setBinaryOutput()
{
    ESP_LOGI(TAG, "Setting binary output");
    outType = BINARY;
    signalStart();
}
int hexToDecimal(char hexChar)
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

size_t hexToByteArray(const char *hexArray, size_t hexLength, uint8_t byteArray[])
{
    size_t i;
    for (i = 0; i < hexLength / 2; i++)
    {
        int highNibble = hexToDecimal(hexArray[i * 2]);
        int lowNibble = hexToDecimal(hexArray[i * 2 + 1]);

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

addrFilter_t filterAddr(char mac[], const size_t readBytes)
{
    addrFilter_t filter = {{}, 0};
    if (readBytes > 0)
    {
        if (readBytes % 2 == 0)
        {
            ESP_LOGI(TAG, "read filter %s bytes read,%d", mac, readBytes);
            size_t t = hexToByteArray(mac, readBytes, filter.addr);
            filter.size = t;
            ESP_LOG_BUFFER_HEX(TAG, filter.addr, filter.size);
        }
        else
        {
            return filter;
        }
    }
    else
    {
        ESP_LOGI(TAG, "No filter");
    }
    return filter;
}

addrFilter_t read_filter(char *op, serial_messsage_t msg)
{
    char mac[12] = "";
    const size_t sz = msg.size - 2;
    strncpy(mac, op + 2, sz);
    addrFilter_t filt = filterAddr(mac, sz);
    return filt;
}

void readMessage(serial_messsage_t msg)
{

    if (msg.size >= OP_CODE_SIZE)
    {

        char *op = (char *)msg.data;
        ESP_LOGI(TAG, "Got %s %d ", op, msg.size);
        if (isOpCode(op, "S0"))
        {
            ESP_LOGI(TAG, "Got stop capture");
            signalStop();
        }

        if (isOpCode(op, "S1"))
        {
            ESP_LOGI(TAG, "Got Start capture");
            signalStart();
        }
        if (isOpCode(op, "FC"))
        {
            const size_t sz = msg.size - 2;
            int ch;
            if (sz > 0)
            {
                char channel[2] = "";
                strncpy(channel, op + 2, sz);
                ch = atoi(channel);
            }
            else
            {
                ch = 0;
            }
            set_filter_channel(ch);
        }

        if (isOpCode(op, "F1"))
        {
            addrFilter_t filt = read_filter(op, msg);
            if (filt.size >= 0)
            {
                setAddr1Filter(filt);
            }
        }

        if (isOpCode(op, "F2"))
        {
            addrFilter_t filt = read_filter(op, msg);
            if (filt.size >= 0)
            {
                setAddr2Filter(filt);
            }
        }
    }
}

void on_serial_read(serial_messsage_t serMsg)
{
    ESP_LOGI(TAG, "port: %d,Data  %s ", serMsg.srcPort, serMsg.data);
    readMessage(serMsg);
}

void onMsgProduce(sent_message_t *msg)
{
    message_t msgFromQueue;

    if (receiveMessage(&msgFromQueue))
    {
        msg->size = msgFromQueue.size;
        memcpy(msg->data, msgFromQueue.data, msgFromQueue.size);

        free(msgFromQueue.data);
        ESP_LOGI(TAG, "Msg Size %d", msg->size);
    }
    else
    {
    }
    // return msg.size;
}

void initSerials()
{
    serial_begin_0(115200); // TODO: make sure this sets back 250000
    // static txConfigStruct_t txConfig = {UART_NUM_0, 100, TX_TASK_SIZE, onMsgProduce};
    //  createTxTask(&txConfig);

    serial_begin_2(115200);
    static rxConfigStruct_t rxConfig = {UART_NUM_0, 10, RX_TASK_SIZE, on_serial_read};
    createRxTask(&rxConfig);
}

#ifdef CONFIG_WIFI_SOFTAP

#define CONFIG_ESP_WIFI_SSID CONFIG_SOFTAP_WIFI_SSID
#define CONFIG_ESP_WIFI_PASS CONFIG_SOFTAP_WIFI_PASSWORD
#define CONFIG_ESP_WIFI_CHANNEL 6 // CONFIG_SOFTAP_WIFI_CHANNEL
#define CONFIG_STATIC_IP_ADDR CONFIG_SOFTAP_WIFI_IP
#define CONFIG_STATIC_GW_ADDR CONFIG_STATIC_IP_ADDR
#define CONFIG_STATIC_NETMASK_ADDR "255.255.255.0"
#define CONFIG_MAIN_DNS_SERVER ""   //"1.1.1.1"
#define CONFIG_BACKUP_DNS_SERVER "" //"8.8.8.8"

#endif

#ifdef CONFIG_WIFI_STA
#define WIFI_SSID CONFIG_STA_WIFI_SSID
#define WIFI_PASS CONFIG_STA_WIFI_PASSWORD
#endif
void init_config_wifi()
{
    wifi_init();

#ifdef CONFIG_WIFI_SOFTAP
    ssid_cfg_t ssid_cfg = {.ssid = CONFIG_ESP_WIFI_SSID, .ssid_sz = strlen(CONFIG_ESP_WIFI_SSID), .password = CONFIG_ESP_WIFI_PASS, .pass_sz = strlen(CONFIG_ESP_WIFI_PASS)};
    wifi_softAP(ssid_cfg, CONFIG_ESP_WIFI_CHANNEL, CONFIG_STATIC_IP_ADDR, CONFIG_STATIC_NETMASK_ADDR, CONFIG_STATIC_GW_ADDR, CONFIG_MAIN_DNS_SERVER, CONFIG_BACKUP_DNS_SERVER);
#endif

#ifdef CONFIG_WIFI_STA
    ssid_cfg_t ssid_cfg_sta = {.ssid = WIFI_SSID, .ssid_sz = strlen(WIFI_SSID), .password = WIFI_PASS, .pass_sz = strlen(WIFI_PASS)};
    wifi_sta(ssid_cfg_sta);
#endif

    char msg[50] = "";
    wifi_get_ip(msg, sizeof(msg));
    displayPrint(0, 17, "IP: %s", msg);
}

#define PORT CONFIG_TCP_SERVER_PORT
#define KEEPALIVE_IDLE CONFIG_TCP_SERVER_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL CONFIG_TCP_SERVER_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT CONFIG_TCP_SERVER_KEEPALIVE_COUNT

void tcp_server()
{
    static tcp_server_config_t tcp_server_config = {.port = PORT, .keepIdle = KEEPALIVE_IDLE, .keepInterval = KEEPALIVE_INTERVAL, .keepCount = KEEPALIVE_COUNT, .on_socket_accept = on_socket_accept_handler};
    start_tcp_server(&tcp_server_config);
}

void setup()
{
    initSerials();
    // esp_log_set_vprintf(serial_log); // REDIRECT ALL LOGS TO another port

    initDisplay();
    //  displayPrint(0, 17, "hello %s", "world");

    ESP_LOGI(TAG, "[+] Startup...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ESP_ERROR_CHECK(esp_event_loop_create(event_handler, NULL)); // initialize (wifi) event handler

    setWriteCb(on_write);
    setPacketHander(writePacket);

    init_config_wifi();

    addrFilter_t ownMac = {.size = 6};
    wifi_get_mac(ownMac.addr);
    ESP_LOGI(TAG, "MAC Address IS : " MACSTR, MAC2STR(ownMac.addr));

    wifi_sniffer_init_config();
    setAddrOwnMacFilter(ownMac);

    static const UBaseType_t txQSize = 5;
    txQueue = createMessageQueue(txQSize);
    if (!txQueue)
    {
        printf("Failed to create queue");
    }
    tcp_server();

    // esp_log_set_vprintf(send_v_msg);//TODO: need to solve on client side
}
static bool RUNNING = true;
void loop()
{
    while (RUNNING)
    { // every 0.5s check if fatal error occurred
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    setup();

    loop();

    ESP_LOGW(TAG, "Deleting sniffing task...");
    sniffer_stop();
    ESP_LOGI(TAG, "Stopped");
}
