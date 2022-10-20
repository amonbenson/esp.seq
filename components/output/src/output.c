#include "output.h"

#include <esp_check.h>
#include <driver/gpio.h>
#include <driver/ledc.h>


static const char *TAG = "output";


esp_err_t output_init(output_t *output, const output_config_t *config) {
    // store the config
    output->config = *config;

    // setup digital channels
    for (int i = 0; i < output->config.digital_pins->length; i++) {
        gpio_num_t pin = output->config.digital_pins->pins[i];
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
        .duty_resolution = LEDC_TIMER_12_BIT,
        .freq_hz = 10000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG,
        "failed to configure pwm timer");

    // setup analog channels
    for (int i = 0; i < output->config.analog_pins->length; i++) {
        gpio_num_t pin = output->config.analog_pins->pins[i];
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

esp_err_t output_set_digital(output_t *output, int channel, int value) {
    // update the digital state
    gpio_num_t pin = output->config.digital_pins->pins[channel];
    ESP_RETURN_ON_ERROR(gpio_set_level(pin, value), TAG,
        "failed to set digital channel %d to %d", channel, value);
    
    return ESP_OK;
}

esp_err_t output_set_analog(output_t *output, int channel, int value) {
    // update the duty cycle
    ESP_RETURN_ON_ERROR(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, channel, value, 0), TAG,
        "failed to set analog (pwm) channel %d to %d", channel, value);
    
    return ESP_OK;
}
