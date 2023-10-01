#include "console_sniffer_cmd.h"

#include "esp_log.h"
#include "sys/errno.h"
#include "esp_console.h"
#include "wifi_sniffer.h"
#include "capture_lib.h"
#include "console_utils.h"

static bool console_read_filter(const char mac[], const size_t readBytes, addrFilter_t *addrFilter)
{
    // addrFilter_t filter = {{}, 0};
    if (readBytes > 0)
    {
        if (readBytes % 2 == 0)
        {
            size_t t = hex_to_byte_array(mac, readBytes, addrFilter->addr);
            addrFilter->size = t;
            printfln("Filtering  %s ", mac);
            return true;
        }
        else if (readBytes > 12)
        {
            printfln("Arugment exceeds 12 bytes %s ", mac);
            return false;
        }
        else
        {
            printfln("Argument must even in length %s ", mac);
            return false;
        }
    }
    else
    {
        addrFilter->size = 0;
        printfln("Clearing filter %s", "");
        return true;
    }
}

static const char *TAG = "ConsoleSnifferCmd";

static struct
{
    struct arg_str *source;
    struct arg_str *frame_type;
    struct arg_int *rssi;
    struct arg_end *end;
} console_sniffer_filter_args;

static void create_sniffer_source_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("s", "source", "<MAC>", "source");
}
static void console_sniffer_filter_source_arg(struct arg_str *arg_source)
{
    if (arg_source->count > 0)
    {
        const char *addr2 = arg_source->sval[0];
        size_t addr2_len = strlen(addr2);

        addrFilter_t filter = {{}, 0};
        if (console_read_filter(addr2, addr2_len, &filter))
        {
            sniffer_set_addr2_filter(filter);
        }
    }
}

static void create_sniffer_frame_arg(struct arg_str **argstr)
{
    *argstr = arg_str0("f", "frame", "<frametype>", "Type: [ D[ata] / A[ll] ]");
}
static void console_sniffer_filter_frame_arg(struct arg_str *arg_frame_type)
{
    if (arg_frame_type->count > 0)
    {
        const char *frame_type = arg_frame_type->sval[0];

        if (strlen(frame_type) == 0 || !strncmp("a", frame_type, 1))
        {
            sniffer_set_no_filter();
        }
        else if (!strncmp("d", frame_type, 1))
        {
            sniffer_set_filter_data();
        }
        else
        {
            printfln("Invalid Frame type %s", frame_type);
        }
    }
}

static void create_sniffer_rssi_arg(struct arg_int **argint)
{
    *argint = arg_int0("r", "rssi", "<rssi>", "rssi");
}
static void console_sniffer_filter_rssi_arg(struct arg_int *arg_rssi)
{
    if (arg_rssi->count > 0)
    {
        rssi_t rssi = arg_rssi->ival[0];
        if (rssi>0){
            printfln("RSSI filter cannot be positive %d",rssi);
        }else{
            sniffer_set_rssi_filter(rssi);
        }
        
    }
}

static int console_sniffer_filter(int argc, char **argv)
{
    console_reset_argstr(console_sniffer_filter_args.source);
    console_reset_argstr(console_sniffer_filter_args.frame_type);
    console_reset_argint(console_sniffer_filter_args.rssi);

    struct arg_end *arg_end = console_sniffer_filter_args.end;

    printfln("\nexecuting");
    CONSOLE_ARGS_PARSE(argc, argv, (void **)&console_sniffer_filter_args, arg_end);
    console_sniffer_filter_source_arg(console_sniffer_filter_args.source);
    console_sniffer_filter_frame_arg(console_sniffer_filter_args.frame_type);
    console_sniffer_filter_rssi_arg(console_sniffer_filter_args.rssi);
    return 0;
}

static void console_sniffer_create_filter_args()
{
    create_sniffer_source_arg(&console_sniffer_filter_args.source);
    create_sniffer_frame_arg(&console_sniffer_filter_args.frame_type);
    create_sniffer_rssi_arg(&console_sniffer_filter_args.rssi);

    console_create_argend(&console_sniffer_filter_args.end);
}
// https://linux.die.net/man/3/argtable
static void console_sniffer_register_filter()
{
    console_sniffer_create_filter_args();
    const esp_console_cmd_t sniffer_filter_cmd = {
        .command = "filter",
        .help = "Filter MACS and frame types\n",
        .hint = NULL,
        .func = &console_sniffer_filter,
        .argtable = &console_sniffer_filter_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&sniffer_filter_cmd));
}

void console_sniffer_register_cmd()
{
    console_sniffer_register_filter();
}
