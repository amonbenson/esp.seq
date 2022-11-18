#pragma once

#include "controller.h"
#include "midi_message.h"


#define CTRL_GENERIC_CC_BPM 1
#define CTRL_GENERIC_BPM_MIN 40
#define CTRL_GENERIC_BPM_MAX 240


extern const controller_class_t controller_class_generic;


typedef struct {
    controller_t super;
} controller_generic_t;


esp_err_t controller_generic_init(controller_generic_t *controller);
esp_err_t controller_generic_free(controller_generic_t *controller);

esp_err_t controller_generic_midi_recv(controller_generic_t *controller, const midi_message_t *message);
