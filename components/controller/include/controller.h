#pragma once

#include "esp_err.h"


typedef struct controller_t controller_t;
typedef esp_err_t (*controller_init_function)(controller_t *controller);
typedef esp_err_t (*controller_free_function)(controller_t *controller);

typedef struct {
    unsigned int size;
    controller_init_function init;
    controller_free_function free;
} controller_class_t;

struct controller_t {
    const controller_class_t *class;
};


controller_t *controller_create(const controller_class_t *class);
esp_err_t controller_free(controller_t *controller);
