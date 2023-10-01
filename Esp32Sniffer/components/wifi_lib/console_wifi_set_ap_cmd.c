#include "console_wifi_set_ap_cmd.h"

#include "console_utils.h"
#include "wifi_lib.h"

static struct
{

    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_int *channel;

    struct arg_end *end;
} console_wifi_set_ap_args;

static void console_wifi_reset_set_ap_args()
{
    console_reset_argstr(console_wifi_set_ap_args.ssid);
    console_reset_argstr(console_wifi_set_ap_args.password);
    console_reset_argint(console_wifi_set_ap_args.channel);
}

static void console_wifi_create_ssid_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("s", "ssid", "<SSID>", "ssid that will be published");
}

static void console_wifi_create_ap_channel_arg(struct arg_int **argint)
{
    *argint = arg_int0("c", "channel", "<channel>", "channel number");
}

static int console_wifi_exec_set_ap(int argc, char **argv)
{
    console_wifi_reset_set_ap_args();

    struct arg_end *arg_end = console_wifi_set_ap_args.end;

    printfln("\nexecuting ap soft");
    CONSOLE_ARGS_PARSE(argc, argv, (void **)&console_wifi_set_ap_args, arg_end);
    
    char ssid[SSID_SZ] = "";
    if (console_wifi_set_ap_args.ssid->count > 0)
    {
        const char *value = console_wifi_set_ap_args.ssid->sval[0];        
        strncpy(ssid, value, sizeof(ssid));
    }
    else
    {
        wifi_nvs_get_ap_ssid(ssid, sizeof(ssid));
    }

    char password[SSID_PASS_SZ] = "";
    if (console_wifi_set_ap_args.password->count > 0)
    {
        const char *value = console_wifi_set_ap_args.password->sval[0];        
        strncpy(password, value, sizeof(password));
    }
    else
    {
        wifi_nvs_get_ap_ssid_pw(password, sizeof(password));
    }
    ssid_cfg_t ssid_cfg=convert_to_ssid_cfg(ssid,sizeof(ssid),password,sizeof(password));
    int ch;
    if (console_wifi_set_ap_args.channel->count > 0)
    {
        ch = console_wifi_set_ap_args.channel->ival[0];
    }
    else
    {
        ch = wifi_nvs_get_ap_channel();
    }
    esp_netif_ip_info_t esp_netif_ip_info=wifi_nvs_get_static_ip_info();
    dns_servers_info_t dns_servers_info=wifi_nvs_get_dns_servers();
    wifi_ap(ssid_cfg,ch,esp_netif_ip_info,dns_servers_info);    
    return 0;
}

static void console_wifi_create_set_ap_args()
{    
    console_wifi_create_ssid_arg(&console_wifi_set_ap_args.ssid);
    console_create_password_arg(&console_wifi_set_ap_args.password);
    console_wifi_create_ap_channel_arg(&console_wifi_set_ap_args.channel);

    console_create_argend(&console_wifi_set_ap_args.end);
}

void console_wifi_register_set_ap_cmd()
{
    console_wifi_create_set_ap_args();

    const esp_console_cmd_t cmd_args = {
        .command = "set-ap",
        .help = "Configure AP ssid and password paramters\n",
        .hint = NULL,
        .func = &console_wifi_exec_set_ap,
        .argtable = &console_wifi_set_ap_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_args));
}