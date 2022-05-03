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


void connected_callback(const usb_device_desc_t *device_descriptor) {
    ESP_LOGI(TAG, "connected (vid: 0x%04x, pid: 0x%04x)", device_descriptor->idVendor, device_descriptor->idProduct);
}

void disconnected_callback(const usb_device_desc_t *device_descriptor) {
    ESP_LOGI(TAG, "disconnected (vid: 0x%04x, pid: 0x%04x)", device_descriptor->idVendor, device_descriptor->idProduct);
}

void sysex_callback(const uint8_t *data, size_t length) {
    ESP_LOGI(TAG, "sysex (length: %d)", length);
}

void note_on_callback(uint8_t channel, uint8_t note, uint8_t velocity) {
    ESP_LOGI(TAG, "note on: channel %d, note %d, velocity %d", channel, note, velocity);

    // echo
    usb_midi_send_note_on(&usb_midi, channel, note, velocity);
}

void note_off_callback(uint8_t channel, uint8_t note, uint8_t velocity) {
    ESP_LOGI(TAG, "note off: channel %d, note %d, velocity %d", channel, note, velocity);

    // echo
    usb_midi_send_note_off(&usb_midi, channel, note, velocity);
}

void poly_key_pressure_callback(uint8_t channel, uint8_t note, uint8_t pressure) {
    ESP_LOGI(TAG, "poly key pressure: channel %d, note %d, pressure %d", channel, note, pressure);
}

void control_change_callback(uint8_t channel, uint8_t controller, uint8_t value) {
    ESP_LOGI(TAG, "control change: channel %d, controller %d, value %d", channel, controller, value);
}

void program_change_callback(uint8_t channel, uint8_t program) {
    ESP_LOGI(TAG, "program change: channel %d, program %d", channel, program);
}

void channel_pressure_callback(uint8_t channel, uint8_t pressure) {
    ESP_LOGI(TAG, "channel pressure: channel %d, pressure %d", channel, pressure);
}

void pitch_bend_callback(uint8_t channel, int16_t value) {
    ESP_LOGI(TAG, "pitch bend: channel %d, value %d", channel, value);
}


void app_main(void) {
    // initialize the dac
    ESP_ERROR_CHECK(dac_global_init());

    // initialize the usb interface
    const usb_midi_config_t usb_midi_config = {
        .callbacks = {
            .connected = connected_callback,
            .disconnected = disconnected_callback,
            .sysex = sysex_callback,
            .note_on = note_on_callback,
            .note_off = note_off_callback,
            .poly_key_pressure = poly_key_pressure_callback,
            .control_change = control_change_callback,
            .program_change = program_change_callback,
            .channel_pressure = channel_pressure_callback,
            .pitch_bend = pitch_bend_callback
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
