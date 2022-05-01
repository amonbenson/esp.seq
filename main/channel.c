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

esp_err_t channel_create(const channel_config_t *config, channel_t *channel) {
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
    err = dac_create(&dac_config, &channel->dac);
    if (err != ESP_OK) return err;

    return ESP_OK;
}
