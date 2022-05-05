#include "midi_interface.h"


esp_err_t midi_interface_init(const midi_interface_config_t *config, midi_interface_t *interface) {
    interface->config = *config;
    return ESP_OK;
}

esp_err_t midi_interface_send(midi_interface_t *interface, const midi_message_t *message) {
    // pass the message down to the underlying interface send handler
    MIDI_INVOKE_CALLBACK(&interface->config.callbacks, send, interface, message, interface->config.callbacks.send_arg);
}

esp_err_t midi_interface_recv(midi_interface_t *interface, const midi_message_t *message) {
    // pass the message up to the application
    MIDI_INVOKE_CALLBACK(&interface->config.callbacks, recv, interface, message, interface->config.callbacks.recv_arg);
}
