#pragma once

#define CLI_NAME_MAX_LEN     64
#define CLI_PASSWORD_MAX_LEN 64

typedef struct {
    char name[CLI_NAME_MAX_LEN];
    char password[CLI_PASSWORD_MAX_LEN];
} wifi_credentials_t;

// Start the UART CLI task. Call once from app_main.
void cli_input_start(void);
