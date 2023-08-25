#include "console_sniffer_cmd.h"

#include "esp_log.h"
#include "sys/errno.h"
#include "esp_console.h"
#include "wifi_sniffer.h"
#include "capture_lib.h"
#include "console_utils.h"

static const char *TAG = "ConsoleSnifferCmd";

static struct
{
    struct arg_str *source;
    struct arg_str *frame_type;
    struct arg_end *end;
} console_sniffer_filter_args;

addrFilter_t console_read_filter(const char mac[], const size_t readBytes)
{
    addrFilter_t filter = {{}, -1};
    if (readBytes > 0)
    {
        if (readBytes % 2 == 0)
        {
            size_t t = hex_to_byte_array(mac, readBytes, filter.addr);
            filter.size = t;
            printfln("Filtering  %s ", mac);
        }
        else if (readBytes > 12)
        {
            printfln("Arugment exceeds 12 bytes %s ", mac);
            return filter;
        }
        else
        {
            printfln("Argument must even in length %s ", mac);
            return filter;
        }
    }
    else
    {
        filter.size = 0;
        printfln("Clearing filter %s", "");
        return filter;
    }

    return filter;
}

static struct arg_str *create_sniffer_source_arg()
{
    return arg_str0("s", "source", "<MAC>", "source");
}
static void console_sniffer_filter_source(struct arg_str *arg_source)
{
    if (arg_source->count > 0)
    {
        const char *addr2 = arg_source->sval[0];
        size_t addr2_len = strlen(addr2);

        addrFilter_t f = console_read_filter(addr2, addr2_len);
        sniffer_set_addr2_filter(f);
    }
}

static struct arg_str *create_sniffer_frame_arg()
{
    return arg_str0("f", "frame", "<frametype>", "Type: [ D[ata] / A[ll] ]");
}
static void console_sniffer_filter_frame(struct arg_str *arg_frame_type)
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

static int console_sniffer_filter(int argc, char **argv)
{
    console_sniffer_filter_args.source->sval[0] = "";
    console_sniffer_filter_args.frame_type->sval[0] = "";
    struct arg_end *arg_end = console_sniffer_filter_args.end;

    if (console_args_parse(argc, argv, (void **)&console_sniffer_filter_args, arg_end))
    {
        return 1;
    }
    else
    {
        console_sniffer_filter_source(console_sniffer_filter_args.source);
        console_sniffer_filter_frame(console_sniffer_filter_args.frame_type);
        return 0;
    }
}
// https://linux.die.net/man/3/argtable
void console_register_sniffer_filter(void)
{
    console_sniffer_filter_args.source = create_sniffer_source_arg();
    console_sniffer_filter_args.frame_type = create_sniffer_frame_arg();
    console_sniffer_filter_args.end = arg_end(2);

    const esp_console_cmd_t sniffer_filter_cmd = {
        .command = "filter",
        .help = "Filter MACS and frame types\n",
        .hint = NULL,
        .func = &console_sniffer_filter,
        .argtable = &console_sniffer_filter_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&sniffer_filter_cmd));
}