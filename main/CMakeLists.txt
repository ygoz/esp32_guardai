idf_component_register(
    SRCS "src/main.c"
         "src/uart/cli_input.c"
    INCLUDE_DIRS "inc"
    PRIV_REQUIRES esp_driver_gpio
                  esp_driver_uart
)
