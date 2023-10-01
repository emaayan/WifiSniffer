
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "sys/errno.h"
#include "console_utils.h"

#define CONSOLE_UART_NUM CONFIG_ESP_CONSOLE_UART_NUM
#define CONSOLE_UART_BUAD 115200
#define CONSOLE_UART_RX_BUFFER_SIZE 256
#define CONSOLE_PROMPT_STR "sniffer> "
#define CONSOLE_PROMPT LOG_COLOR_I CONSOLE_PROMPT_STR LOG_RESET_COLOR

static const char *TAG = "ConsoleLib";

void console_begin()
{

    console_register_common();
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = CONSOLE_PROMPT;

    esp_console_repl_t *repl = NULL;

#if CONFIG_ESP_CONSOLE_UART
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t usbjtag_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usbjtag_config, &repl_config, &repl));
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

static int console_set_line_endings(esp_line_endings_t esp_line_endings)
{
    int err_line_ends = esp_vfs_dev_uart_port_set_rx_line_endings(CONSOLE_UART_NUM, esp_line_endings);
    if (err_line_ends != 0)
    {
        ESP_LOGE(TAG, "Error on setting : errno %d", errno);
    }
    return err_line_ends;
}

static void console_serial_begin(uart_port_t port, int baud) // TODO: externlize serial driver config
{
    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
        .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(port, CONSOLE_UART_RX_BUFFER_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
}

void console_config_init(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    console_set_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    console_set_line_endings(ESP_LINE_ENDINGS_CRLF);

    console_serial_begin(CONSOLE_UART_NUM, CONSOLE_UART_BUAD);

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
        // #if CONFIG_LOG_COLORS
        //         .hint_color = atoi(LOG_COLOR_CYAN)
        // #endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

    /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    /* Don't return empty lines */
    linenoiseAllowEmpty(false);

    // #if CONFIG_STORE_HISTORY
    //     /* Load command history from filesystem */
    //     linenoiseHistoryLoad(HISTORY_PATH);
    // #endif
    ESP_ERROR_CHECK(esp_console_register_help_command());
}

// https://docs.espressif.com/projects/esp-idf/en/v5.0.3/esp32/api-reference/system/console.html

void console_start()
{
    const char *prompt = CONSOLE_PROMPT;
    printf("\n"
           "This is an example of ESP-IDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB!!! when typing command name to auto-complete.\n"
           "Press Enter or Ctrl+C will terminate the console environment.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status)
    { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
        // #if CONFIG_LOG_COLORS
        //         /* Since the terminal doesn't support escape sequences,
        //          * don't use color codes in the prompt.
        //          */
        //         prompt = CONSOLE_PROMPT_STR;
        // #endif // CONFIG_LOG_COLORS
    }

    /* Main loop */
    while (true)
    {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char *line = linenoise(prompt);
        if (line == NULL)
        { /* Break on EOF or error */
            break;
        }
        /* Add the command to the history if not empty*/
        if (strlen(line) > 0)
        {
            linenoiseHistoryAdd(line);
            // #if CONFIG_STORE_HISTORY
            //             /* Save command history to filesystem */
            //             linenoiseHistorySave(HISTORY_PATH);
            // #endif
        }

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unrecognized command\n");
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            // command was empty
        }
        else if (err == ESP_OK && ret != ESP_OK)
        {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        }
        else if (err != ESP_OK)
        {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");

    esp_console_deinit();
}
