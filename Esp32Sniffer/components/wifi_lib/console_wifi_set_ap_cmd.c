#include "console_wifi_set_ap_cmd.h"

#include "console_utils.h"
#include "wifi_lib.h"


static struct
{
    struct arg_lit *enable;
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


static void console_wifi_set_ap_enable_arg(struct arg_lit *arg_enable)
{
    if (arg_enable->count > 0)
    {        
        printfln("Enabled Soft AP");
        wifi_set_mode(WIFI_LIB_MODE_AP);
    }
}

static void console_wifi_create_ssid_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("s", "ssid", "<SSID>", "ssid the sniffer will publish");
}
static void console_wifi_set_ap_ssid_arg(struct arg_str *arg_ssid)
{
    if (arg_ssid->count > 0)
    {
        const char *value = arg_ssid->sval[0];
        size_t len = strlen(value);
        wifi_nvs_set_ap_ssid(value, len);
    }
}


static void console_wifi_set_ap_password_arg(struct arg_str *arg_password)
{
    if (arg_password->count > 0)
    {
        const char *value = arg_password->sval[0];
        size_t len = strlen(value);
        wifi_nvs_set_ap_ssid_pw(value, len);
    }
}

static void console_wifi_create_ap_channel_arg(struct arg_int **argint)
{
    *argint = arg_int0("c", "channel", "<channel>", "channel number");
}
static void console_wifi_set_ap_channel_arg(struct arg_int *arg_channel)
{
    if (arg_channel->count > 0)
    { // TODO: MAKE VALIDATION OF CHANNEL RANGE
        int value = arg_channel->ival[0];
        wifi_nvs_set_ap_channel(value);
    }
}

static int console_wifi_exec_set_ap(int argc, char **argv)
{
    console_wifi_reset_set_ap_args();

    struct arg_end *arg_end = console_wifi_set_ap_args.end;

    printfln("\nexecuting ap soft");
    CONSOLE_ARGS_PARSE(argc, argv, (void **)&console_wifi_set_ap_args, arg_end);

    console_wifi_set_ap_ssid_arg(console_wifi_set_ap_args.ssid);
    console_wifi_set_ap_password_arg(console_wifi_set_ap_args.password);
    console_wifi_set_ap_channel_arg(console_wifi_set_ap_args.channel);
    console_wifi_set_ap_enable_arg(console_wifi_set_ap_args.enable); // if enabled also execute actuall command with paramters
    return 0;
}

static void console_wifi_create_set_ap_args()
{
    console_create_enable_arg(&console_wifi_set_ap_args.enable);
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