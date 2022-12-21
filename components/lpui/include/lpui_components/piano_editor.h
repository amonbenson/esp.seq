#pragma once

#include "lpui.h"


typedef struct {
    lpui_component_config_t cmp_config;
} piano_editor_config_t;

typedef struct {
    piano_editor_config_t config;
    lpui_component_t cmp;
} piano_editor_t;


esp_err_t piano_editor_init(piano_editor_t *editor, const piano_editor_config_t *config);
esp_err_t piano_editor_update(piano_editor_t *editor);
esp_err_t piano_editor_key_event(void *context, const lpui_position_t pos, uint8_t velocity);
