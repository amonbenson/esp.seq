#include "output.h"

#include <esp_check.h>
#include <driver/gpio.h>
#include <driver/ledc.h>


static const char *TAG = "output";


static esp_err_t output_port_analog_channel_claim(output_t *output, output_port_t *port) {
    ESP_RETURN_ON_FALSE(port->config.type == OUTPUT_ANALOG, ESP_ERR_INVALID_STATE, TAG,
        "port %d is not analog", port->index);
    ESP_RETURN_ON_FALSE(port->analog_channel == -1, ESP_ERR_INVALID_STATE, TAG,
        "port %d has already claimed channel %d", port->index, port->analog_channel);

    // check if we find an empty channel (index == -1)
    for (ledc_channel_t ch = 0; ch < OUTPUT_MAX_ANALOG_PORTS; ch++) {
        if (output->claimed_analog_channels[ch] == -1) {
            output->claimed_analog_channels[ch] = port->index;
            port->analog_channel = ch;
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "no free analog channels");
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t output_port_analog_channel_release(output_t *output, output_port_t *port) {
    ESP_RETURN_ON_FALSE(port->config.type == OUTPUT_ANALOG, ESP_ERR_INVALID_ARG, TAG,
        "port %d is not analog", port->index);
    ESP_RETURN_ON_FALSE(port->analog_channel != -1, ESP_ERR_INVALID_ARG, TAG,
        "port %d has not claimed an analog channel", port->index);

    int8_t claimed_port = output->claimed_analog_channels[port->analog_channel];
    ESP_RETURN_ON_FALSE(claimed_port == port->index, ESP_ERR_INVALID_STATE, TAG,
        "analog channel %d is not claimed by port %d", port->analog_channel, port->index);

    // set both mappings to -1
    output->claimed_analog_channels[port->analog_channel] = -1;
    port->analog_channel = -1;

    return ESP_OK;
}

static esp_err_t output_port_setup_digital(output_t *output, output_port_t *port) {
    // setup gpio pin
    port->analog_channel = -1;
    gpio_config_t digital_config = {
        .pin_bit_mask = 1ULL << port->config.pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&digital_config), TAG,
        "failed to configure digital port %d on pin %d", port->index, port->config.pin);
    
    return ESP_OK;
}

static esp_err_t output_port_setup_analog(output_t *output, output_port_t *port) {
    // claim a free analog channel
    ESP_RETURN_ON_ERROR(output_port_analog_channel_claim(output, port), TAG,
        "failed to claim analog channel for port %d", port->index);

    // setup ledc pwm channel
    ledc_channel_config_t analog_config = {
        .gpio_num = port->config.pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = port->analog_channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&analog_config), TAG,
        "failed to configure analog port %d on pin %d, channel %d",
        port->index, port->config.pin, port->analog_channel);
    
    return ESP_OK;
}

static esp_err_t output_port_setup(output_t *output, output_port_t *port) {
    if (port->config.type == OUTPUT_DIGITAL) {
        return output_port_setup_digital(output, port);
    } else {
        return output_port_setup_analog(output, port);
    }
}

static esp_err_t output_port_teardown_digital(output_t *output, output_port_t *port) {
    // nothing to do
    return ESP_OK;
}

static esp_err_t output_port_teardown_analog(output_t *output, output_port_t *port) {
    // release the analog channel
    ESP_RETURN_ON_ERROR(output_port_analog_channel_release(output, port), TAG,
        "failed to release analog channel for port %d", port->index);

    // stop the ledc pwm channel
    ESP_RETURN_ON_ERROR(ledc_stop(LEDC_LOW_SPEED_MODE, port->analog_channel, 0), TAG,
        "failed to stop analog port %d on pin %d, channel %d",
        port->index, port->config.pin, port->analog_channel);

    return ESP_OK;
}

static esp_err_t output_port_teardown(output_t *output, output_port_t *port) {
    if (port->config.type == OUTPUT_DIGITAL) {
        return output_port_teardown_digital(output, port);
    } else {
        return output_port_teardown_analog(output, port);
    }
}

esp_err_t output_init(output_t *output, const output_config_t *config) {
    // store the config
    output->config = *config;

    // store the total number of ports
    output->num_ports = config->num_rows * config->num_columns;
    ESP_RETURN_ON_FALSE(output->num_ports == 0 || output->num_ports <= OUTPUT_MAX_PORTS,
        ESP_ERR_INVALID_ARG, TAG,
        "invalid number of ports %d", output->num_ports);
    
    // clear the claimed analog channels array
    for (int i = 0; i < OUTPUT_MAX_ANALOG_PORTS; i++) {
        output->claimed_analog_channels[i] = -1;
    }
    
    // setup the pwm timer for the analog ports
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = OUTPUT_PWM_DUTY_RESOLUTION,
        .freq_hz = OUTPUT_PWM_FREQUENCY,
        .clk_cfg = LEDC_USE_APB_CLK
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG,
        "failed to configure pwm timer");

    // setup all ports
    output->ports = malloc(output->num_ports * sizeof(output_port_t));
    ESP_RETURN_ON_FALSE(output->ports, ESP_ERR_NO_MEM, TAG,
        "failed to allocate memory for %d ports", output->num_ports);

    for (uint8_t x = 0; x < output->config.num_columns; x++) {
        for (uint8_t y = 0; y < output->config.num_rows; y++) {
            uint8_t i = x * output->config.num_rows + y;
            output_port_t *port = &output->ports[i];
            port->config = config->port_configs[i];
            port->index = i;
            port->column = x;
            port->row = y;

            port->analog_channel = -1;
            port->value_mv = 0;

            // setup the port (configures the gpio and ledc channels)
            ESP_RETURN_ON_ERROR(output_port_setup(output, port), TAG,
                "failed to setup port %d", i);
            
            // set initial voltage level
            ESP_RETURN_ON_ERROR(output_port_set_voltage(output, port, port->value_mv), TAG,
                "failed to set initial voltage level for port %d", i);
        }
    }

    return ESP_OK;
}

output_port_t *output_port_get(output_t *output, uint8_t column, uint8_t row) {
    if (column >= output->config.num_columns || row >= output->config.num_rows) {
        ESP_LOGE(TAG, "invalid port %d, %d", column, row);
        return NULL;
    }

    uint8_t index = row * output->config.num_columns + column;
    return &output->ports[index];
}

esp_err_t output_port_set_type(output_t *output, output_port_t *port, output_type_t type) {
    // if the type is the same, do nothing
    if (port->config.type == type) return ESP_OK;

    // teardown, re-type, setup
    ESP_RETURN_ON_ERROR(output_port_teardown(output, port), TAG,
        "failed to teardown port %d", port->index);
    port->config.type = type;
    ESP_RETURN_ON_ERROR(output_port_setup(output, port), TAG,
        "failed to setup port %d", port->index);

    // set the output voltage
    output_port_set_voltage(output, port, port->value_mv);

    return ESP_OK;
}

static esp_err_t output_port_set_voltage_digital(output_t *output, output_port_t *port, uint32_t value_mv) {
    // set the gpio level
    uint32_t level = value_mv > 0;
    ESP_LOGD(TAG, "port %d (pin %d, digital) => %dmV",
        port->index, port->config.pin, level);
    ESP_RETURN_ON_ERROR(gpio_set_level(port->config.pin, level), TAG,
        "failed to set digital port %d to %d", port->index, value_mv);

    port->value_mv = value_mv;
    return ESP_OK;
}

static esp_err_t output_port_set_voltage_analog(output_t *output, output_port_t *port, uint32_t value_mv) {
    ledc_channel_t channel = port->analog_channel;

    // scale the voltage to duty cycle resolution
    uint32_t duty = value_mv * OUTPUT_PWM_DUTY_MAX / port->config.vmax_mv;

    // upper clamping
    if (duty > OUTPUT_PWM_DUTY_MAX) {
        duty = OUTPUT_PWM_DUTY_MAX;
    }

    // update the duty cycle
    ESP_LOGD(TAG, "port %d (pin %d, analog chan %d) => %dmV",
        port->index, port->config.pin, channel, value_mv);
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty), TAG,
        "failed to set analog port %d (channel %d) to %d mV (%d%% duty)",
        port->index, channel, value_mv, duty * 100 / OUTPUT_PWM_DUTY_MAX);
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel), TAG,
        "failed to update analog port %d", port->index);
    
    return ESP_OK;
}

esp_err_t output_port_set_voltage(output_t *output, output_port_t *port, uint32_t value_mv) {
    // clamp the value
    if (value_mv > port->config.vmax_mv) {
        value_mv = port->config.vmax_mv;
    }

    // set the voltage level
    if (port->config.type == OUTPUT_DIGITAL) {
        return output_port_set_voltage_digital(output, port, value_mv);
    } else {
        return output_port_set_voltage_analog(output, port, value_mv);
    }
}

esp_err_t output_set_type(output_t *output, uint8_t column, uint8_t row, output_type_t type) {
    output_port_t *port = output_port_get(output, column, row);
    if (!port) return ESP_ERR_INVALID_ARG;

    return output_port_set_type(output, port, type);
}
    

esp_err_t output_set_voltage(output_t *output, uint8_t column, uint8_t row, uint32_t value_mv) {
    output_port_t *port = output_port_get(output, column, row);
    if (!port) return ESP_ERR_INVALID_ARG;

    return output_port_set_voltage(output, port, value_mv);
}
