#pragma once

#include <esp_err.h>
#include <driver/ledc.h>


#define OUTPUT_PWM_DUTY_RESOLUTION LEDC_TIMER_10_BIT // -> PWM freq = 20kHz
#define OUTPUT_PWM_DUTY_MAX ((1U << OUTPUT_PWM_DUTY_RESOLUTION) - 1)
#define OUTPUT_PWM_FREQUENCY (5000 * (1U << (12 - OUTPUT_PWM_DUTY_RESOLUTION)))

#define OUTPUT_MAX_ANALOG_PORTS LEDC_CHANNEL_MAX
#define OUTPUT_MAX_PORTS GPIO_NUM_MAX

//#define OUTPUT_ANALOG_MILLIVOLTS 4840 // 5.5V = 3.3V * 1.67 gain


typedef enum {
    OUTPUT_DIGITAL,
    OUTPUT_ANALOG
} output_type_t;

typedef struct {
    output_type_t type;
    gpio_num_t pin;
    uint32_t vmax_mv;
} output_port_config_t;

typedef struct {
    output_port_config_t config;
    uint8_t index, row, column;

    ledc_channel_t analog_channel;
    uint32_t value_mv;
} output_port_t;

typedef struct {
    uint8_t num_columns;
    uint8_t num_rows;
    const output_port_config_t *port_configs;
} output_config_t;

typedef struct {
    output_config_t config;

    uint8_t num_ports;
    output_port_t *ports;

    int8_t claimed_analog_channels[OUTPUT_MAX_ANALOG_PORTS];
} output_t;


esp_err_t output_init(output_t *output, const output_config_t *config);

output_port_t *output_port_get(output_t *output, uint8_t column, uint8_t row);
esp_err_t output_port_set_type(output_t *output, output_port_t *port, output_type_t type);
esp_err_t output_port_set_voltage(output_t *output, output_port_t *port, uint32_t value_mv);

esp_err_t output_set_type(output_t *output, uint8_t column, uint8_t row, output_type_t type);
esp_err_t output_set_voltage(output_t *output, uint8_t column, uint8_t row, uint32_t value_mv);
