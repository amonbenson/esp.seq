#include "controllers/generic.h"
#include <esp_log.h>


static const char *TAG = "generic controller";

const controller_class_t controller_class_generic = {
    .size = sizeof(controller_generic_t),
    .init = controller_generic_init,
    .free = controller_generic_free,
    .midi_recv = controller_generic_midi_recv
};


esp_err_t controller_generic_init(controller_generic_t *controller) {
    ESP_LOGI(TAG, "init");

    return ESP_OK;
}

esp_err_t controller_generic_free(controller_generic_t *controller) {
    ESP_LOGI(TAG, "free");

    return ESP_OK;
}

esp_err_t controller_generic_midi_recv(controller_generic_t *controller, const midi_message_t *message) {
    ESP_LOGI(TAG, "Controller Received Message: cmd: %02x, chan: %02x, data: %02x %02x",
        message->command,
        message->channel,
        message->body[0],
        message->body[1]);

    switch (message->command) {
        case MIDI_COMMAND_CONTROL_CHANGE:
            switch (message->control_change.control) {
                case CTRL_GENERIC_CC_BPM:;
                    // change bpm
                    uint16_t bpm = CTRL_GENERIC_BPM_MIN +
                        ((uint16_t) message->control_change.value * (CTRL_GENERIC_BPM_MAX - CTRL_GENERIC_BPM_MIN) / 127);
                    sequencer_set_bpm(controller->super.sequencer,
                        bpm);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return ESP_OK;
}
