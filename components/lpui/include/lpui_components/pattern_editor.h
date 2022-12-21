#pragma once

#include "lpui.h"
#include "callback.h"
#include "sequencer.h"


typedef struct pattern_editor_t pattern_editor_t;
CALLBACK_DECLARE(pattern_editor_step_select, esp_err_t,
    pattern_editor_t *editor, uint16_t step_position);

typedef struct {
    struct {
        void *context;
        CALLBACK_TYPE(pattern_editor_step_select) step_selected;
    } callbacks;
    lpui_component_config_t cmp_config;
} pattern_editor_config_t;

struct pattern_editor_t {
    pattern_editor_config_t config;
    lpui_component_t cmp;

    int track_id;
    pattern_t *pattern;

    uint8_t page;
    uint16_t step_offset;

    uint16_t step_position;
};


esp_err_t pattern_editor_init(pattern_editor_t *editor, const pattern_editor_config_t *config);
esp_err_t pattern_editor_key_event(void *context, const lpui_position_t pos, uint8_t velocity);

esp_err_t pattern_editor_draw(pattern_editor_t *editor);
esp_err_t pattern_editor_draw_steps(pattern_editor_t *editor, uint16_t step_positions[], size_t n);

esp_err_t pattern_editor_set_pattern(pattern_editor_t *editor, int track_id, pattern_t *pattern);
esp_err_t pattern_editor_update_step_position(pattern_editor_t *editor);
