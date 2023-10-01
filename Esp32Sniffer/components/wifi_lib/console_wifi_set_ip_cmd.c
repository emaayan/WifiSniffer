#include "console_wifi_set_ip_cmd.h"

#include "console_utils.h"
#include "wifi_lib.h"

static struct
{
    struct arg_str *ip;

    struct arg_end *end;
} console_wifi_set_ip_args;

static void console_wifi_reset_set_ip_args()
{
    console_reset_argstr(console_wifi_set_ip_args.ip);
}

static void console_wifi_create_ip_arg(struct arg_str **argstr)
{
    *argstr = arg_str1("i", "ip", "<ip>", "static ip");
}
static void console_wifi_set_ip_ip_arg(struct arg_str *arg_ip)
{
    if (arg_ip->count > 0)
    {
        if(wifi_nvs_get_mode()!=WIFI_LIB_MODE_AP)
        {
            printfln("can only set static ip in ap mode");
            return ;
        }
        const char *value = arg_ip->sval[0];        
        esp_netif_ip_info_t ip = convert_to_ip(value);
        
        if (wifi_is_valid_ip(ip.ip))
        {            
            wifi_conf_set_static_ip(ip);                       
        }
        else
        {
            printfln("Invalid IP setting %s",value);
        }
    }
}

static int console_wifi_exec_set_ip(int argc, char **argv)
{
    console_wifi_reset_set_ip_args();
    struct arg_end *arg_end = console_wifi_set_ip_args.end;

    printfln("\nexecuting setting ip");
    CONSOLE_ARGS_PARSE(argc, argv, (void **)&console_wifi_set_ip_args, arg_end);
    console_wifi_set_ip_ip_arg(console_wifi_set_ip_args.ip);
    return 0;
}

static void console_wifi_create_set_ip_args()
{
    console_wifi_create_ip_arg(&console_wifi_set_ip_args.ip);

    console_create_argend(&console_wifi_set_ip_args.end);
}

void console_wifi_register_set_ip_cmd()
{
    console_wifi_create_set_ip_args();

    const esp_console_cmd_t cmd_args = {
        .command = "set-ip",
        .help = "Configure IP Paramters (subnet will be 255.255.255.0) gw will be ip\n",
        .hint = NULL,
        .func = &console_wifi_exec_set_ip,
        .argtable = &console_wifi_set_ip_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_args));
}
