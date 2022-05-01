#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "dac.h"
#include "channel.h"


#define NUM_CHANNELS 4


static const char *TAG = "espmidi";

const channel_config_t channel_configs[NUM_CHANNELS] = {
    {
        .gate_pin = GPIO_NUM_6,
        .trigger_pin = GPIO_NUM_7,
        .dac_pin = GPIO_NUM_1
    },
    {
        .gate_pin = GPIO_NUM_8,
        .trigger_pin = GPIO_NUM_9,
        .dac_pin = GPIO_NUM_2
    },
    {
        .gate_pin = GPIO_NUM_10,
        .trigger_pin = GPIO_NUM_11,
        .dac_pin = GPIO_NUM_3
    },
    {
        .gate_pin = GPIO_NUM_12,
        .trigger_pin = GPIO_NUM_13,
        .dac_pin = GPIO_NUM_34
    }
};

static channel_t channels[NUM_CHANNELS];


void app_main(void) {
    ESP_ERROR_CHECK(dac_global_init());

    /* dac_t dac;
    const dac_config_t dac_config = {
        .cs_pin = GPIO_NUM_1
    };
    
    if (dac_create(&dac_config, &dac) != ESP_OK) {
        ESP_LOGE(TAG, "failed to create dac");
        return;
    }

    int i = 0;
    while (1) {
        for (; i < 1000; i++) {
            dac_set_value_mapped(&dac, DAC_A, i, 1000);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        for (; i >= 0; i--) {
            dac_set_value_mapped(&dac, DAC_A, i, 1000);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    } */

    // create all channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        ESP_ERROR_CHECK(channel_create(&channel_configs[i], &channels[i]));
    }

    while (1) {
        ESP_ERROR_CHECK(dac_set_value_mapped(&channels[0].dac, DAC_A, 70, 100));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(dac_set_value_mapped(&channels[0].dac, DAC_A, 90, 100));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

   vTaskDelete(NULL);
}
