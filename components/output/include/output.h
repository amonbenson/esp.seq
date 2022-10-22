#pragma once

#include <esp_err.h>
#include <driver/ledc.h>


#define OUTPUT_PWM_DUTY_RESOLUTION LEDC_TIMER_12_BIT
#define OUTPUT_PWM_FREQUENCY 5000

#define OUTPUT_ANALOG_RANGE (1ULL << OUTPUT_PWM_DUTY_RESOLUTION)
#define OUTPUT_ANALOG_MILLIVOLTS 5000


typedef struct {
    const gpio_num_t *digital_pins;
    uint8_t num_digital_pins;

    const gpio_num_t *analog_pins;
    uint8_t num_analog_pins;

    struct {
        uint8_t columns;
        uint8_t rows;
    } matrix_size;
} output_config_t;

typedef struct {
    output_config_t config;
} output_t;


esp_err_t output_init(output_t *output, const output_config_t *config);

esp_err_t output_set(output_t *output, uint8_t column, uint8_t row, uint32_t value);

esp_err_t output_set_digital(output_t *output, uint8_t channel, uint32_t value);
esp_err_t output_set_analog(output_t *output, uint8_t channel, uint32_t millivolts);
esp_err_t output_set_analog_raw(output_t *output, uint8_t channel, uint32_t value);
