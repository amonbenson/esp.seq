#include "channel.h"
#include <string.h>
#include <esp_log.h>


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
    esp_err_t err;

    // store the config
    memcpy(&channel->config, config, sizeof(channel_config_t));

    // setup the gate and trigger gpios
    ESP_LOGI(TAG, "configuring gate and trigger gpios");
    err = channel_gpio_config(config->gate_pin);
    if (err != ESP_OK) return err;

    err = channel_gpio_config(config->trigger_pin);
    if (err != ESP_OK) return err;

    // setup the note / velocity dac
    ESP_LOGI(TAG, "configuring dac");
    const dac_config_t dac_config = {
        .cs_pin = config->dac_pin
    };
    err = dac_init(&dac_config, &channel->dac);
    if (err != ESP_OK) return err;

    return ESP_OK;
}

esp_err_t channel_set_note(channel_t *channel, uint8_t note) {
    return channel_set_note_cents(channel, (uint16_t) note * 100);
}

esp_err_t channel_set_note_cents(channel_t *channel, uint16_t cents) {
    uint16_t chan_vmax = 10560;

    // value = V / Vmax * DACmax, with V = cents * (5/6)
    uint16_t value = (uint16_t) (cents * (uint32_t) (DAC_TOTAL_RESOLUTION - 1) * 5 / 6 / chan_vmax);
    if (value > DAC_TOTAL_RESOLUTION - 1) value = DAC_TOTAL_RESOLUTION - 1;

    return dac_set_value(&channel->dac, DAC_A, value);
}

esp_err_t channel_set_velocity(channel_t *channel, uint8_t velocity) {
    uint16_t value = (uint16_t) velocity * (DAC_TOTAL_RESOLUTION - 1) / 127;
    return dac_set_value(&channel->dac, DAC_B, value);
}

esp_err_t channel_set_gate(channel_t *channel, bool gate) {
    return gpio_set_level(channel->config.gate_pin, gate);
}

esp_err_t channel_set_trigger(channel_t *channel, bool trigger) {
    return gpio_set_level(channel->config.trigger_pin, trigger);
}
