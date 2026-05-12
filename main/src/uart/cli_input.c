#include "uart/cli_input.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define CLI_UART       UART_NUM_0
#define RX_BUF_SIZE    512
#define LINE_BUF_SIZE  256
#define TASK_STACK     4096
#define TASK_PRIO      2        // Low: spends most of its time blocked on uart_read_bytes

// printf via uart_write_bytes — works correctly after uart_driver_install
static void cli_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_write_bytes(CLI_UART, buf, len);
}

static void parse_and_respond(char *line)
{
    wifi_credentials_t creds = {0};

    char *tok = strtok(line, " \t");
    while (tok) {
        char *val = strtok(NULL, " \t");

        if ((strcmp(tok, "--name") == 0 || strcmp(tok, "-n") == 0)
                && val && val[0] != '-') {
            strncpy(creds.name, val, sizeof(creds.name) - 1);
            tok = strtok(NULL, " \t");

        } else if ((strcmp(tok, "--password") == 0 || strcmp(tok, "-p") == 0)
                && val && val[0] != '-') {
            strncpy(creds.password, val, sizeof(creds.password) - 1);
            tok = strtok(NULL, " \t");

        } else {
            tok = val;
        }
    }

    cli_printf("Responding...\r\n");

    if (!creds.name[0] && !creds.password[0]) {
        cli_printf("  Usage: --name <ssid> --password <pass>\r\n"
                   "         -n <ssid> -p <pass>\r\n");
        return;
    }

    cli_printf("  Name:     %s\r\n", creds.name[0]     ? creds.name     : "(not set)");
    cli_printf("  Password: %s\r\n", creds.password[0] ? creds.password : "(not set)");
}

static void cli_task(void *arg)
{
    const uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_driver_install(CLI_UART, RX_BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CLI_UART, &cfg));

    char line[LINE_BUF_SIZE];
    int  pos = 0;
    int  last_was_cr = 0;

    cli_printf("\r\nCLI ready. Enter: -n <ssid> -p <password>\r\n> ");

    while (1) {
        uint8_t ch;
        if (uart_read_bytes(CLI_UART, &ch, 1, portMAX_DELAY) != 1) continue;

        // swallow the \n half of a \r\n pair so CRLF doesn't produce a double prompt
        if (ch == '\n' && last_was_cr) { last_was_cr = 0; continue; }
        last_was_cr = (ch == '\r');

        uart_write_bytes(CLI_UART, (const char *)&ch, 1); // local echo

        if (ch == '\r' || ch == '\n') {
            if (pos == 0) {
                cli_printf("\r\n> ");
                continue;
            }
            line[pos] = '\0';
            pos = 0;
            cli_printf("\r\n");
            parse_and_respond(line);
            cli_printf("> ");

        } else if ((ch == 127 || ch == '\b') && pos > 0) {
            pos--;
            uart_write_bytes(CLI_UART, "\b \b", 3); // erase char on terminal

        } else if (ch >= 0x20 && pos < LINE_BUF_SIZE - 1) {
            line[pos++] = (char)ch;
        }
    }
}

void cli_input_start(void)
{
    xTaskCreate(cli_task, "cli_input", TASK_STACK, NULL, TASK_PRIO, NULL);
}
