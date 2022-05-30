#pragma once

#include <esp_err.h>
#include "pattern.h"


#define TRACK_MAX_PATTERNS 16


#define TRACK_DEFAULT_CONFIG() ((track_config_t) { })


typedef struct {
} track_config_t;

typedef struct {
    track_config_t config;
    uint32_t playhead;

    pattern_t patterns[TRACK_MAX_PATTERNS];
    int active_pattern;
} track_t;


esp_err_t track_init(track_t *track, const track_config_t *config);

esp_err_t track_seek(track_t *track, uint32_t playhead);
esp_err_t track_tick(track_t *track, uint32_t playhead);

esp_err_t track_set_active_pattern(track_t *track, int pattern_id);
pattern_t *track_get_active_pattern(track_t *track);
