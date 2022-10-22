#include "output.h"

#include <esp_check.h>
#include <driver/gpio.h>
#include <driver/ledc.h>


static const char *TAG = "output";


esp_err_t output_init(output_t *output, const output_config_t *config) {
    // validate the config
    ESP_RETURN_ON_FALSE(config->num_analog_pins <= LEDC_CHANNEL_MAX,
        ESP_ERR_INVALID_ARG, TAG, "too many analog pins");
    ESP_RETURN_ON_FALSE(config->num_analog_pins + config->num_digital_pins == config->matrix_size.rows * config->matrix_size.columns,
        ESP_ERR_INVALID_ARG, TAG, "matrix size does not match number of pins");

    // store the config
    output->config = *config;

    // setup digital channels
    for (int i = 0; i < output->config.num_digital_pins; i++) {
        gpio_num_t pin = output->config.digital_pins[i];
        gpio_config_t config = {
            .pin_bit_mask = 1ULL << pin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_RETURN_ON_ERROR(gpio_config(&config), TAG,
            "failed to configure digital pin %d", pin);
    }

    // setup the pwm timer for the analog channels
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = OUTPUT_PWM_DUTY_RESOLUTION,
        .freq_hz = OUTPUT_PWM_FREQUENCY,
        .clk_cfg = LEDC_USE_APB_CLK
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG,
        "failed to configure pwm timer");

    // setup analog channels
    for (int i = 0; i < output->config.num_analog_pins; i++) {
        gpio_num_t pin = output->config.analog_pins[i];
        ledc_channel_config_t channel_config = {
            .gpio_num = pin,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = i,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0
        };
        ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_config), TAG,
            "failed to configure analog (pwm) pin %d", pin);
    }

    return ESP_OK;
}

esp_err_t output_set(output_t *output, uint8_t column, uint8_t row, uint32_t value) {    
    ESP_RETURN_ON_FALSE(row < output->config.matrix_size.rows, ESP_ERR_INVALID_ARG, TAG,
        "row %d out of bounds", row);
    ESP_RETURN_ON_FALSE(column < output->config.matrix_size.columns, ESP_ERR_INVALID_ARG, TAG,
        "column %d out of bounds", column);

    uint8_t index = row * output->config.matrix_size.columns + column;
    if (index < output->config.num_analog_pins) {
        return output_set_analog(output, index, value);
    } else {
        return output_set_digital(output, index - output->config.num_analog_pins, value);
    }
}

esp_err_t output_set_digital(output_t *output, uint8_t channel, uint32_t value) {
    value = !!value;

    // update the digital state
    //ESP_LOGI(TAG, "DIGITAL %d = %d", channel, value);
    gpio_num_t pin = output->config.digital_pins[channel];
    ESP_RETURN_ON_ERROR(gpio_set_level(pin, value), TAG,
        "failed to set digital channel %d to %d", channel, value);
    
    return ESP_OK;
}

esp_err_t output_set_analog(output_t *output, uint8_t channel, uint32_t millivolts) {
    uint32_t value = millivolts * (OUTPUT_ANALOG_RANGE - 1) / OUTPUT_ANALOG_MILLIVOLTS;
    return output_set_analog_raw(output, channel, value );
}

esp_err_t output_set_analog_raw(output_t *output, uint8_t channel, uint32_t value) {
    if (value > OUTPUT_ANALOG_RANGE - 1) {
        ESP_LOGW(TAG, "analog value %d is out of range, clamping to %llu", value, OUTPUT_ANALOG_RANGE - 1);
        value = OUTPUT_ANALOG_RANGE - 1;
    }

    // update the duty cycle
    //ESP_LOGI(TAG, "ANALOG %d = %d", channel, value);
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, value), TAG,
        "ledc_set_duty failed on analog (pwm) channel %d to %d", channel, value);
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel), TAG,
        "ledc_update_duty failed on analog (pwm) channel %d", channel);
    
    return ESP_OK;
}
