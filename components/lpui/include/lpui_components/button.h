#pragma once

#include "lpui.h"
#include <stdbool.h>


typedef struct button_t button_t;
CALLBACK_DECLARE(button_pressed, esp_err_t, button_t *button);
CALLBACK_DECLARE(button_released, esp_err_t, button_t *button);


typedef enum {
    BUTTON_MODE_MOMENTARY,
    BUTTON_MODE_TOGGLE
} button_mode_t;

typedef struct {
    struct {
        void *context;
        CALLBACK_TYPE(button_pressed) pressed;
        CALLBACK_TYPE(button_released) released;
    } callbacks;
    lpui_component_config_t cmp_config;

    lpui_color_t color;
    button_mode_t mode;
} button_config_t;

struct button_t {
    button_config_t config;
    lpui_component_t cmp;

    bool pressed;
};


esp_err_t button_init(button_t *button, const button_config_t *config);
esp_err_t button_key_event(void *context, const lpui_position_t pos, uint8_t velocity);

esp_err_t button_draw(button_t *button);

esp_err_t button_set_pressed(button_t *button, bool pressed);
