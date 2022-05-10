#include "midi.h"


/* static void midi_interface_recv_callback(void *arg, const midi_message_t *message) {
    midi_t *midi = (midi_t *) arg;
}

esp_err_t midi_init(const midi_config_t *config, midi_t *midi) {
    midi->config = *config;
    return ESP_OK;
}

esp_err_t midi_send(midi_t *midi, size_t interface_id, const midi_message_t *message) {
    if (interface_id >= midi->config.num_interfaces) {
        return ESP_ERR_INVALID_ARG;
    }

    MIDI_INVOKE_CALLBACK(&midi->config.callbacks, send, interface_id, message);
    return ESP_OK;
} */
