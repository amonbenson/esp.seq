#pragma once

#include <esp_err.h>
#include <driver/gpio.h>
#include <dac.h>


typedef struct {
    gpio_num_t gate_pin;
    gpio_num_t trigger_pin;
    gpio_num_t dac_pin;
} channel_config_t;

typedef struct {
    channel_config_t config;
    dac_t dac;
} channel_t;


esp_err_t channel_create(const channel_config_t *config, channel_t *channel);
