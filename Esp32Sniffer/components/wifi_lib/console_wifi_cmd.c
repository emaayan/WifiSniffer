#include "console_wifi_cmd.h"

#include "console_wifi_set_ip_cmd.h"
#include "console_wifi_set_ap_cmd.h"
#include "console_wifi_set_sta_cmd.h"


void console_wifi_register_cmd()
{
    console_wifi_register_set_sta_cmd();
    console_wifi_register_set_ip_cmd();
    console_wifi_register_set_ap_cmd();
}