#pragma once

#include <esp_err.h>
#include "sequencer_config.h"


#define PATTERN_DEFAULT_CONFIG() ((pattern_config_t) { \
    .step_length = 16, \
    .resolution = SEQ_TICKS_PER_SIXTEENTH_NOTE \
})

#define PATTERN_NUM_NOTES 4


typedef struct {
    uint8_t notes[PATTERN_NUM_NOTES];
    uint8_t velocity;
} pattern_step_t;


typedef struct {
    uint16_t step_length;
    uint16_t resolution;
} pattern_config_t;

typedef struct {
    pattern_config_t config;
    int id;

    uint16_t substep_position;
    uint16_t step_position;

    pattern_step_t *steps;
} pattern_t;


esp_err_t pattern_init(pattern_t *pattern, const pattern_config_t *config);

esp_err_t pattern_resize(pattern_t *pattern, uint16_t num_steps);
esp_err_t pattern_seek(pattern_t *pattern, uint32_t playhead);
esp_err_t pattern_tick(pattern_t *pattern);

pattern_step_t *pattern_get_active_step(pattern_t *pattern);
