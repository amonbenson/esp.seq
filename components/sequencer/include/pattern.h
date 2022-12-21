#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include "sequencer_config.h"
#include "callback.h"


#define PATTERN_DEFAULT_CONFIG() ((pattern_config_t) { \
    .type = PATTERN_TYPE_MELODIC, \
    .step_length = 16, \
    .resolution = SEQ_TICKS_PER_SIXTEENTH_NOTE \
})


typedef union {
    struct {
        uint8_t note;
        uint8_t velocity;
    };
    uint16_t drum_mask;
} pattern_atomic_step_t;

typedef struct {
    pattern_atomic_step_t atomic;
    uint8_t gate;
    uint8_t probability;
} pattern_step_t;


typedef enum {
    PATTERN_TYPE_MELODIC,
    PATTERN_TYPE_DRUM
} pattern_type_t;

typedef struct {
    pattern_type_t type;
    uint16_t step_length;
    uint16_t resolution;
} pattern_config_t;

typedef struct {
    pattern_config_t config;
    int id;

    uint16_t substep_position;
    uint16_t step_position;

    bool active_step_enabled;
    uint16_t active_step_off;

    pattern_step_t *steps;
    pattern_atomic_step_t state;
} pattern_t;


esp_err_t pattern_init(pattern_t *pattern, const pattern_config_t *config);

esp_err_t pattern_resize(pattern_t *pattern, uint16_t num_steps);
esp_err_t pattern_seek(pattern_t *pattern, uint32_t playhead);
esp_err_t pattern_tick(pattern_t *pattern);

pattern_step_t *pattern_get_active_step(pattern_t *pattern);
pattern_step_t *pattern_get_previous_step(pattern_t *pattern);
pattern_step_t *pattern_get_next_step(pattern_t *pattern);

uint32_t pattern_step_to_ticks(pattern_t *pattern, uint16_t step);
uint16_t pattern_ticks_to_step(pattern_t *pattern, uint32_t ticks);
