#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "dac.h"
#include "usb.h"
#include "usb_midi.h"
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

static usb_midi_t usb_midi;
static channel_t channels[NUM_CHANNELS];


void connected_callback(usb_device_desc_t *device_descriptor) {
    ESP_LOGI(TAG, "connected (vid: 0x%04x, pid: 0x%04x)", device_descriptor->idVendor, device_descriptor->idProduct);
}

void disconnected_callback(usb_device_desc_t *device_descriptor) {
    ESP_LOGI(TAG, "disconnected (vid: 0x%04x, pid: 0x%04x)", device_descriptor->idVendor, device_descriptor->idProduct);
}

void note_on_callback(uint8_t channel, uint8_t note, uint8_t velocity) {
    ESP_LOGI(TAG, "note on: channel %d, note %d, velocity %d", channel, note, velocity);
}

void note_off_callback(uint8_t channel, uint8_t note, uint8_t velocity) {
    ESP_LOGI(TAG, "note off: channel %d, note %d, velocity %d", channel, note, velocity);
}


void app_main(void) {
    // initialize the dac
    ESP_ERROR_CHECK(dac_global_init());

    // initialize the usb interface
    const usb_midi_config_t usb_midi_config = {
        .callbacks = {
            .connected = connected_callback,
            .disconnected = disconnected_callback,
            .note_on = note_on_callback,
            .note_off = note_off_callback
        }
    };
    ESP_ERROR_CHECK(usb_midi_init(&usb_midi_config, &usb_midi));
    ESP_ERROR_CHECK(usb_init(&usb_midi.driver_config));

    // create all channels
    /* for (int i = 0; i < NUM_CHANNELS; i++) {
        ESP_ERROR_CHECK(channel_init(&channel_configs[i], &channels[i]));
    } */

    vTaskDelete(NULL);
}
