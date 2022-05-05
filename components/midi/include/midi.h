#pragma once

#include <esp_err.h>
#include "usb_midi.h"
#include "midi_message.h"
// #include "midi_interface.h"


typedef void (*midi_send_callback_t)(size_t interface_id, const midi_message_t *message);
typedef void (*midi_recv_callback_t)(size_t interface_id, const midi_message_t *message);

typedef struct {
    struct {
        midi_recv_callback_t recv;
        midi_send_callback_t send;
    } callbacks;

    size_t num_interfaces;
} midi_config_t;

typedef struct {
    midi_config_t config;
} midi_t;


esp_err_t midi_init(const midi_config_t *config, midi_t *midi);

esp_err_t midi_send(midi_t *midi, size_t interface_id, const midi_message_t *message);

esp_err_t midi_interface_recv(midi_t *midi, size_t interface_id, const midi_message_t *message);
