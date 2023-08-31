#include "console_wifi_cmd.h"

#include "esp_log.h"
#include "sys/errno.h"
#include "esp_console.h"
#include "wifi_sniffer.h"
#include "console_utils.h"
#include "nvs.h"

static struct
{
    struct arg_lit *enable;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} console_wifi_ap_args;

static void create_wifi_ap_enable_arg(struct arg_lit **arglit)
{
    *arglit = arg_lit0("e", "enable", "enable");
}

static void console_wifi_ap_enable_arg(struct arg_lit *arg_enable)
{
    if (arg_enable->count > 0)
    {
        printfln("Enabled sofTAP");
    }
}

static void create_wifi_ap_ssid_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("s", "ssid", "<SSID>", "ssid the sniffer will publish");
}

static void console_wifi_ap_ssid_arg(struct arg_str *arg_ssid)
{
    if (arg_ssid->count > 0)
    {
        const char *value = arg_ssid->sval[0];
       //size_t len = strlen(value);

        printfln("%s %s", arg_ssid->hdr.longopts, value); // save to flash
    }
}

static void create_wifi_ap_password_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("p", "password", "<password>", "password for the ssid");
}

static void console_wifi_ap_password_arg(struct arg_str *arg_password)
{
    if (arg_password->count > 0)
    {
        const char *value = arg_password->sval[0];
      //  size_t len = strlen(value);

        printfln("%s %s", arg_password->hdr.longopts, value); // save to flash
    }
}

static int console_wifi_exec_set_wifi_ap(int argc, char **argv)
{
    console_reset_argstr(console_wifi_ap_args.ssid);
    console_reset_argstr(console_wifi_ap_args.password);

    struct arg_end *arg_end = console_wifi_ap_args.end;

    printfln("\nexecuting");    
    CONSOLE_ARGS_PARSE(argc, argv, (void **)&console_wifi_ap_args, arg_end);
    console_wifi_ap_enable_arg(console_wifi_ap_args.enable);
    console_wifi_ap_ssid_arg(console_wifi_ap_args.ssid);
    console_wifi_ap_password_arg(console_wifi_ap_args.password);
    return 0;
}

static void console_wifi_create_ap_args()
{
    create_wifi_ap_enable_arg(&console_wifi_ap_args.enable);
    create_wifi_ap_ssid_arg(&console_wifi_ap_args.ssid);
    create_wifi_ap_password_arg(&console_wifi_ap_args.password);

    console_reset_argend(&console_wifi_ap_args.end);
}

void console_wifi_register_ap_cmd()
{
    console_wifi_create_ap_args();

    const esp_console_cmd_t cmd_args = {
        .command = "set-softap",
        .help = "Configure AP ssid and password paramters\n",
        .hint = NULL,
        .func = &console_wifi_exec_set_wifi_ap,
        .argtable = &console_wifi_ap_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_args));
}