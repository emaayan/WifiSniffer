#include "console_utils.h"

#include "sys/errno.h"
#include <stdarg.h>
#include "esp_system.h"
#include "esp_chip_info.h"

int console_args_parse(int argc, char **argv, void **argtable, struct arg_end *arg_end)
{
    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, arg_end, argv[0]);
    }
    return nerrors;
}
void console_reset_argstr(struct arg_str *argstr)
{
    argstr->sval[0] = "";
}

void console_reset_argend(struct arg_end **argend)
{
    *argend = arg_end(2);
}

void printfln(const char *fmt, ...)
{
    char buffer[200] = "";
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, argptr);
    va_end(argptr);

    printf("%s\n", buffer);
}

/* 'version' command */
static int get_version(int argc, char **argv)
{

    esp_chip_info_t info;
    esp_chip_info(&info);

    const char *model;
    switch (info.model)
    {
    case CHIP_ESP32:
        model = "ESP32";
        break;
    case CHIP_ESP32S2:
        model = "ESP32-S2";
        break;
    case CHIP_ESP32S3:
        model = "ESP32-S3";
        break;
    case CHIP_ESP32C3:
        model = "ESP32-C3";
        break;
    case CHIP_ESP32H2:
        model = "ESP32-H2";
        break;
    case CHIP_ESP32C2:
        model = "ESP32-C2";
        break;
    default:
        model = "Unknown";
        break;
    }
    // uint32_t flash_size;
    //  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    //  {
    //      printf("Get flash size failed");
    //      return 1;
    //  }
    printfln("IDF Version:%s", esp_get_idf_version());
    printfln("Chip info:");
    printfln("\tmodel:%s", model);
    printfln("\tcores:%d", info.cores);
    // printf("\tfeature:%s%s%s%s%" PRIu32 "%s\r\n",
    //        info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
    //        info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
    //        info.features & CHIP_FEATURE_BT ? "/BT" : "",
    //        info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
    //        flash_size / (1024 * 1024), " MB");
    printfln("\trevision number:%d", info.revision);
    return 0;
}
void console_register_version(void)
{
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Get version of chip and SDK",
        .hint = NULL,
        .func = &get_version,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
void console_register_common()
{
    esp_console_register_help_command();
    console_register_version();
}