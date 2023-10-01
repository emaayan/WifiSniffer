#include "console_wifi_set_sta_cmd.h"

#include "console_utils.h"
#include "wifi_lib.h"

static struct
{

    struct arg_str *ssid;
    struct arg_str *password;

    struct arg_end *end;
} console_wifi_set_sta_args;

static void console_wifi_reset_set_sta_args()
{
    console_reset_argstr(console_wifi_set_sta_args.ssid);
    console_reset_argstr(console_wifi_set_sta_args.password);
}

static void console_wifi_create_ssid_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("s", "ssid", "<SSID>", "ssid the sniffer will publish");
}

static int console_wifi_exec_set_sta(int argc, char **argv)
{
    console_wifi_reset_set_sta_args();

    struct arg_end *arg_end = console_wifi_set_sta_args.end;

    printfln("\nexecuting setting sta");
    CONSOLE_ARGS_PARSE(argc, argv, (void **)&console_wifi_set_sta_args, arg_end);

    char ssid[SSID_SZ] = "";
    if (console_wifi_set_sta_args.ssid->count > 0)
    {
        const char *value = console_wifi_set_sta_args.ssid->sval[0];        
        strncpy(ssid,value,sizeof(ssid));
    }
    else
    {
        wifi_nvs_get_sta_ssid(ssid, sizeof(ssid));
    }
    char password[SSID_PASS_SZ] = "";
    if (console_wifi_set_sta_args.password->count > 0)
    {
        const char *value = console_wifi_set_sta_args.password->sval[0];        
        strncpy(password,value,sizeof(password));
    }
    else
    {
        wifi_nvs_get_sta_ssid_pw(password, sizeof(password));
    }

    ssid_cfg_t  ssid_cfg=convert_to_ssid_cfg(ssid, sizeof(ssid), password, sizeof(password));
    wifi_sta(ssid_cfg);

    return 0;
}

static void console_wifi_create_set_sta_args()
{
    
    console_wifi_create_ssid_arg(&console_wifi_set_sta_args.ssid);
    console_create_password_arg(&console_wifi_set_sta_args.password);

    console_create_argend(&console_wifi_set_sta_args.end);
}

void console_wifi_register_set_sta_cmd()
{
    console_wifi_create_set_sta_args();

    const esp_console_cmd_t cmd_args = {
        .command = "set-sta",
        .help = "Configure to join ssid and password paramters\n",
        .hint = NULL,
        .func = &console_wifi_exec_set_sta,
        .argtable = &console_wifi_set_sta_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_args));
}
