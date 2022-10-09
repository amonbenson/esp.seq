#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "dac.h"
#include "usb.h"
#include "midi_message.h"
#include "usb_midi.h"
#include "launchpad.h"
#include "channel.h"
#include "sequencer.h"


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
static launchpad_t launchpad;
static channel_t channels[NUM_CHANNELS];
static sequencer_t sequencer;


void usb_midi_connected_callback(const usb_device_desc_t *desc) {
    if (desc->idVendor == LAUNCHPAD_VENDOR_ID && desc->idProduct == LAUNCHPAD_PRO_PRODUCT_ID) {
        launchpad_connected_callback(&launchpad, desc);
    }
}

void usb_midi_disconnected_callback(const usb_device_desc_t *desc) {
    if (launchpad.connected) {
        launchpad_disconnected_callback(&launchpad);
    }
}

void usb_midi_recv_callback(const midi_message_t *message) {
    if (launchpad.connected) {
        launchpad_recv_callback(&launchpad, message);
    }

    if (message->command == MIDI_COMMAND_NOTE_ON) {
        channel_set_note(&channels[0], message->note_on.note);
        channel_set_velocity(&channels[0], message->note_on.velocity);
        channel_set_gate(&channels[0], true);
    } else if (message->command == MIDI_COMMAND_NOTE_OFF && channels[0].note == message->note_off.note) {
        channel_set_velocity(&channels[0], 0);
        channel_set_gate(&channels[0], false);
    }
}

void sequencer_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    uint32_t playhead;
    pattern_atomic_step_t *step;

    if (event_base == SEQUENCER_EVENT) {
        switch (event_id) {
            case SEQUENCER_TICK_EVENT:
                playhead = *(uint32_t *) event_data;
                //ESP_LOGI(TAG, "tick %d", playhead);
                break;
            case SEQUENCER_PLAY_EVENT:
                ESP_LOGI(TAG, "play");
                break;
            case SEQUENCER_PAUSE_EVENT:
                ESP_LOGI(TAG, "pause");
                break;
            case SEQUENCER_SEEK_EVENT:
                playhead = *(uint32_t *) event_data;
                ESP_LOGI(TAG, "seek %d", playhead);
                break;
            default:
                ESP_LOGE(TAG, "unknown sequencer event: %d", event_id);
                break;
        }
    }

    if (event_base == TRACK_EVENT) {
        switch (event_id) {
            case TRACK_NOTE_ON_EVENT:
                step = (pattern_atomic_step_t *) event_data;
                ESP_LOGI(TAG, "note on %d %d", step->note, step->velocity);
                break;
            case TRACK_NOTE_OFF_EVENT:
                step = (pattern_atomic_step_t *) event_data;
                ESP_LOGI(TAG, "note off %d", step->note);
                break;
            default:
                ESP_LOGE(TAG, "unknown track event: %d", event_id);
                break;
        }
    }
}


void app_main(void) {
    // initialize the dac interface
    ESP_ERROR_CHECK(dac_global_init());

    // initialize the usb interface
    const usb_midi_config_t usb_midi_config = {
        .callbacks = {
            .connected = usb_midi_connected_callback,
            .disconnected = usb_midi_disconnected_callback,
            .recv = usb_midi_recv_callback
        }
    };
    ESP_ERROR_CHECK(usb_midi_init(&usb_midi_config, &usb_midi));
    ESP_ERROR_CHECK(usb_init(&usb_midi.driver_config));

    // initialize the launchpad controller
    const launchpad_config_t launchpad_config = {
        .usb_midi = &usb_midi
    };
    ESP_ERROR_CHECK(launchpad_init(&launchpad_config, &launchpad));

    // initialize all output channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        ESP_ERROR_CHECK(channel_init(&channel_configs[i], &channels[i]));
    }

    // initialize the sequencer
    const sequencer_config_t sequencer_config = {
        .bpm = 120,
        .event_handler = sequencer_event_handler,
        .event_handler_arg = NULL
    };
    ESP_ERROR_CHECK(sequencer_init(&sequencer, &sequencer_config));
    ESP_ERROR_CHECK(track_set_active_pattern(&sequencer.tracks[0], 0));
    ESP_ERROR_CHECK(sequencer_play(&sequencer));

    // test sequence
    pattern_t *p = track_get_active_pattern(&sequencer.tracks[0]);
    for (uint16_t i = 0; i < p->config.step_length; i++) {
        p->steps[i].state.note = 60 + i;
        p->steps[i].state.velocity = (i % 2 == 0) ? 127 : 0;
    }

    // start the dac interface
    ESP_ERROR_CHECK(dac_start());

    // app is now running, we can delete the setup task
    vTaskDelete(NULL);
}
