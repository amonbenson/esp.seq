#pragma once

#include <esp_err.h>
#include "pattern.h"


#define TRACK_MAX_PATTERNS 16


typedef enum {
    TRACK_NOTE_CHANGE,
    TRACK_VELOCITY_CHANGE
} track_event_t;

typedef struct track_t track_t;
CALLBACK_DECLARE(track_event, esp_err_t,
    track_event_t event, track_t *track, void *data);


typedef struct {
    struct {
        void *context;
        CALLBACK_TYPE(track_event) event;
    } callbacks;
} track_config_t;

struct track_t {
    track_config_t config;
    uint32_t playhead;

    pattern_t patterns[TRACK_MAX_PATTERNS];

    int active_pattern;
    pattern_atomic_step_t active_step;
};


esp_err_t track_init(track_t *track, const track_config_t *config);

esp_err_t track_seek(track_t *track, uint32_t playhead);
esp_err_t track_tick(track_t *track, uint32_t playhead);

esp_err_t track_set_active_pattern(track_t *track, int pattern_id);
pattern_t *track_get_active_pattern(track_t *track);
