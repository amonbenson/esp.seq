#include "controllers/generic.h"
#include <esp_log.h>


static const char *TAG = "generic controller";

const controller_class_t controller_class_generic = {
    .size = sizeof(controller_generic_t),
    .supported = (controller_supported_function) controller_generic_supported,
    .init = (controller_init_function) controller_generic_init,
    .free = (controller_free_function) controller_generic_free,
    .midi_recv = (controller_midi_recv_function) controller_generic_midi_recv,
    .sequencer_event = (controller_sequencer_event_function) controller_generic_sequencer_event
};


bool controller_generic_supported(const usb_device_desc_t *desc) {
    // generic controller supports all midi devices
    return true;
}


esp_err_t controller_generic_init(controller_generic_t *controller) {
    // reset the current note
    controller->current_note = -1;

    // halt the sequencer
    sequencer_pause(controller->super.config.sequencer);

    return ESP_OK;
}

esp_err_t controller_generic_free(controller_generic_t *controller) {
    // halt the sequencer
    sequencer_pause(controller->super.config.sequencer);

    return ESP_OK;
}

static void controller_generic_note_off(controller_generic_t *controller, uint8_t note) {
    // release the note only if it is the current one
    if (controller->current_note == note) {
        output_set_voltage(controller->super.config.output, 0, 1, OUTPUT_VELOCITY_TO_VOLTAGE(0));
        controller->current_note = -1;
    }
}

static void controller_generic_note_on(controller_generic_t *controller, uint8_t note, uint8_t velocity) {
    // same as note off
    if (velocity == 0) {
        controller_generic_note_off(controller, note);
        return;
    }

    // set the current note
    output_set_voltage(controller->super.config.output, 0, 0, OUTPUT_NOTE_TO_VOLTAGE(note));
    output_set_voltage(controller->super.config.output, 0, 1, OUTPUT_VELOCITY_TO_VOLTAGE(velocity));
    controller->current_note = note;
}

esp_err_t controller_generic_midi_recv(controller_generic_t *controller, const midi_message_t *message) {
    ESP_LOGI(TAG, "Controller Received Message: cmd: %02x, chan: %02x, data: %02x %02x",
        message->command,
        message->channel,
        message->body[0],
        message->body[1]);

    switch (message->command) {
        case MIDI_COMMAND_NOTE_ON:
            controller_generic_note_on(controller, message->note_on.note, message->note_on.velocity);
            break;

        case MIDI_COMMAND_NOTE_OFF:
            controller_generic_note_off(controller, message->note_off.note);
            break;

        /* case MIDI_COMMAND_CONTROL_CHANGE:
            switch (message->control_change.control) {
                case GENERIC_CC_BPM:;
                    // change bpm
                    uint16_t bpm = GENERIC_BPM_MIN +
                        ((uint16_t) message->control_change.value * (GENERIC_BPM_MAX - GENERIC_BPM_MIN) / 127);
                    sequencer_set_bpm(controller->super.sequencer,
                        bpm);
                    break;
                default:
                    break;
            }
            break; */

        default:
            break;
    }

    return ESP_OK;
}

esp_err_t controller_generic_sequencer_event(controller_generic_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    /* if (event_base == SEQUENCER_EVENT) {
        switch (event_id) {
            case SEQUENCER_TICK_EVENT:;
                uint32_t playhead = *(uint32_t *) event_data;
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
    } */

    return ESP_OK;
}
