#pragma once

#include "esp_err.h"
#include "midi_message.h"
#include "sequencer.h"
#include "output.h"


typedef struct controller_t controller_t;
typedef esp_err_t (*controller_init_function)(void *controller);
typedef esp_err_t (*controller_free_function)(void *controller);
typedef esp_err_t (*controller_midi_recv_function)(void *controller, const midi_message_t *message);

typedef struct {
    unsigned int size;
    controller_init_function init;
    controller_free_function free;
    controller_midi_recv_function midi_recv;
} controller_class_t;

struct controller_t {
    const controller_class_t *class;
    sequencer_t *sequencer;
    output_t *output;
};


controller_t *controller_create(const controller_class_t *class, sequencer_t *sequencer, output_t *output);
esp_err_t controller_free(controller_t *controller);

esp_err_t controller_midi_recv(controller_t *controller, const midi_message_t *message);
