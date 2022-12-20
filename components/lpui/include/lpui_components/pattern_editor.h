#pragma once

#include "lpui.h"
#include "pattern.h"


typedef struct {
    lpui_component_t cmp;
    uint8_t page;

    int track_id;
    pattern_t *pattern;
    uint16_t step_position;
} lpui_pattern_editor_t;


esp_err_t lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern);
esp_err_t lpui_pattern_editor_set_track_id(lpui_pattern_editor_t *editor, int track_id);
