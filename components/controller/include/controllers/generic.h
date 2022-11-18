#pragma once

#include "controller.h"


extern const controller_class_t controller_class_generic;


typedef struct {
    controller_t super;
} controller_generic_t;


esp_err_t controller_generic_init(controller_t *controller);
esp_err_t controller_generic_free(controller_t *controller);
