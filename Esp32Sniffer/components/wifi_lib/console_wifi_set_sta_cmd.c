#include "console_wifi_set_sta_cmd.h"

#include "console_utils.h"
#include "wifi_lib.h"


static struct
{
    struct arg_lit *enable;
    struct arg_str *ssid;
    struct arg_str *password;

    struct arg_end *end;
} console_wifi_set_sta_args;

static void console_wifi_reset_set_sta_args()
{
    console_reset_argstr(console_wifi_set_sta_args.ssid);
    console_reset_argstr(console_wifi_set_sta_args.password);
}

static void console_wifi_set_sta_enable_arg(struct arg_lit *arg_enable)
{
    if (arg_enable->count > 0)
    {
        wifi_set_mode(WIFI_LIB_MODE_STA);
        printfln("Enabled sta");
    }
}

static void console_wifi_create_ssid_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("s", "ssid", "<SSID>", "ssid the sniffer will publish");
}
static void console_wifi_set_sta_ssid_arg(struct arg_str *arg_ssid)
{
    if (arg_ssid->count > 0)
    {
        const char *value = arg_ssid->sval[0];
        size_t len = strlen(value);
        wifi_nvs_set_sta_ssid(value, len);
    }
}

static void console_wifi_set_sta_password_arg(struct arg_str *arg_password)
{
    if (arg_password->count > 0)
    {
        const char *value = arg_password->sval[0];
        size_t len = strlen(value);
        wifi_nvs_set_sta_ssid_pw(value, len);
    }
}

static int console_wifi_exec_set_sta(int argc, char **argv)
{
    console_wifi_reset_set_sta_args();

    struct arg_end *arg_end = console_wifi_set_sta_args.end;

    printfln("\nexecuting setting sta");
    CONSOLE_ARGS_PARSE(argc, argv, (void **)&console_wifi_set_sta_args, arg_end);
    console_wifi_set_sta_ssid_arg(console_wifi_set_sta_args.ssid);
    console_wifi_set_sta_password_arg(console_wifi_set_sta_args.password);
    console_wifi_set_sta_enable_arg(console_wifi_set_sta_args.enable); // if enabled also execute command with paramters
    return 0;
}

static void console_wifi_create_set_sta_args()
{
    console_create_enable_arg(&console_wifi_set_sta_args.enable);
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
