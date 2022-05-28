#include "channel.h"
#include <string.h>
#include <esp_log.h>
#include <esp_check.h>


static const char *TAG = "channel";


static esp_err_t channel_gpio_config(gpio_num_t pin) {
    const gpio_config_t config = {
        .pin_bit_mask = (1 << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    return gpio_config(&config);
}

esp_err_t channel_init(const channel_config_t *config, channel_t *channel) {
    // store the config
    channel->config = *config;

    // setup the gate and trigger gpios
    ESP_RETURN_ON_ERROR(channel_gpio_config(config->gate_pin),
        TAG, "failed to configure gate gpio");

    ESP_RETURN_ON_ERROR(channel_gpio_config(config->trigger_pin),
        TAG, "failed to configure trigger gpio");

    // setup the note / velocity dac
    const dac_config_t dac_config = {
        .cs_pin = config->dac_pin
    };
    ESP_RETURN_ON_ERROR(dac_init(&dac_config, &channel->dac),
        TAG, "failed to initialize dac");
    
    // set the initial values
    channel->note = 0;
    channel->cents = 0;
    channel->velocity = 0;
    channel->gate = 0;
    channel->trigger = 0;

    return ESP_OK;
}

esp_err_t channel_set_note(channel_t *channel, uint8_t note) {
    return channel_set_note_cents(channel, (uint16_t) note * 100);
}

esp_err_t channel_set_note_cents(channel_t *channel, uint16_t cents) {
    uint16_t chan_vmax = 10560;

    if (channel->cents == cents) return ESP_OK;
    channel->cents = cents;
    channel->note = cents / 100;

    // value = V / Vmax * DACmax, with V = cents * (5/6)
    uint16_t value = (uint16_t) (cents * (uint32_t) (DAC_TOTAL_RESOLUTION - 1) * 5 / 6 / chan_vmax);
    ESP_LOGD(TAG, "cents = %d --> value = %d/%d", cents, value, DAC_TOTAL_RESOLUTION - 1);
    if (value > DAC_TOTAL_RESOLUTION - 1) value = DAC_TOTAL_RESOLUTION - 1;

    return dac_set_value(&channel->dac, DAC_A, value);
}

esp_err_t channel_set_velocity(channel_t *channel, uint8_t velocity) {
    if (channel->velocity == velocity) return ESP_OK;
    channel->velocity = velocity;

    uint16_t value = (uint16_t) velocity * (DAC_TOTAL_RESOLUTION - 1) / 127;
    ESP_LOGD(TAG, "velocity = %d --> value = %d", velocity, value);
    return dac_set_value(&channel->dac, DAC_B, value);
}

esp_err_t channel_set_gate(channel_t *channel, bool gate) {
    if (channel->gate == gate) return ESP_OK;
    channel->gate = gate;

    return gpio_set_level(channel->config.gate_pin, gate);
}

esp_err_t channel_set_trigger(channel_t *channel, bool trigger) {
    if (channel->trigger == trigger) return ESP_OK;
    channel->trigger = trigger;

    return gpio_set_level(channel->config.trigger_pin, trigger);
}
