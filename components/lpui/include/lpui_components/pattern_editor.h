#pragma once

#include "lpui.h"
#include "sequencer.h"


typedef struct {
    lpui_component_config_t cmp_config;
    sequencer_t *sequencer;
} pattern_editor_config_t;

typedef struct {
    pattern_editor_config_t config;
    lpui_component_t cmp;

    uint8_t page;
    uint16_t step_offset;
    int track_id;

    struct {
        pattern_t *pattern;
        uint8_t page;
        uint16_t step_position;
    } previous;
} pattern_editor_t;


esp_err_t pattern_editor_init(pattern_editor_t *editor, const pattern_editor_config_t *config);
esp_err_t pattern_editor_update(pattern_editor_t *editor);
esp_err_t pattern_editor_button_event(void *context, const lpui_position_t pos, uint8_t velocity);

esp_err_t pattern_editor_set_track_id(pattern_editor_t *editor, int track_id);
