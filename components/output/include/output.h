#pragma once

#include <esp_err.h>
#include <driver/ledc.h>

typedef struct {
    int length;
    gpio_num_t pins[];
} output_pins_t;

typedef struct {
    const output_pins_t *digital_pins;
    const output_pins_t *analog_pins;
} output_config_t;

typedef struct {
    output_config_t config;
} output_t;


esp_err_t output_init(output_t *output, const output_config_t *config);

esp_err_t output_set_analog(output_t *output, int channel, int value);
esp_err_t output_set_digital(output_t *output, int channel, int value);
