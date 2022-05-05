#pragma once

#include "midi_message.h"


typedef (*midi_interface_send_callback)(midi_interface_t *interface, const midi_message_t *message, void *arg);
typedef (*midi_interface_recv_callback)(midi_interface_t *interface, const midi_message_t *message, void *arg);

typedef struct {
    struct {
        midi_interface_send_callback send;
        void *send_arg;
        midi_interface_recv_callback recv;
        void *recv_arg;
    } callbacks;
} midi_interface_config_t;

typedef struct {
    midi_interface_config_t config;
} midi_interface_t;


esp_err_t midi_interface_init(const midi_interface_config_t *config, midi_interface_t *interface);

esp_err_t midi_interface_send(midi_interface_t *interface, const midi_message_t *message);

esp_err_t midi_interface_recv(midi_interface_t *interface, const midi_message_t *message);
