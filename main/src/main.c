#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "dac.h"
#include "usb.h"
#include "midi_message.h"
#include "usb_midi.h"
#include "channel.h"


#define NUM_CHANNELS 4


//static const char *TAG = "espmidi";


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


void usb_midi_recv_callback(const midi_message_t *message) {
    if (message->command != MIDI_COMMAND_SYSEX) midi_message_print(message);
    usb_midi_send(&usb_midi, message);
}


void app_main(void) {
    // initialize the dac
    ESP_ERROR_CHECK(dac_global_init());

    // initialize the usb interface
    const usb_midi_config_t usb_midi_config = {
        .callbacks = {
            .recv = usb_midi_recv_callback
        }
    };
    ESP_ERROR_CHECK(usb_midi_init(&usb_midi_config, &usb_midi));
    ESP_ERROR_CHECK(usb_init(&usb_midi.driver_config));

    // create all channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        ESP_ERROR_CHECK(channel_init(&channel_configs[i], &channels[i]));
    }

    vTaskDelete(NULL);
}
