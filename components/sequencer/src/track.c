#include "track.h"
#include <esp_check.h>


static const char *TAG = "sequencer: track";


esp_err_t track_init(track_t *track, const track_config_t *config) {
    track->config = *config;
    track->active_pattern = -1;
    track->active_step = (pattern_atomic_step_t) { .note = 0, .velocity = 0 };

    track_seek(track, 0);

    // initialize all patterns
    const pattern_config_t pattern_config = PATTERN_DEFAULT_CONFIG();
    for (uint8_t i = 0; i < TRACK_MAX_PATTERNS; i++) {
        ESP_RETURN_ON_ERROR(pattern_init(&track->patterns[i], &pattern_config),
            TAG, "failed to initialize pattern %d", i);
    }

    return ESP_OK;
}

esp_err_t track_seek(track_t *track, uint32_t playhead) {
    track->playhead = playhead;

    pattern_t *pattern = track_get_active_pattern(track);
    if (pattern) return pattern_seek(pattern, playhead);

    return ESP_OK;
}

esp_err_t track_tick(track_t *track, uint32_t playhead) {
    esp_err_t ret;
    pattern_t *pattern = track_get_active_pattern(track);
    if (pattern == NULL) return ESP_OK;

    // update the pattern's state
    ESP_RETURN_ON_ERROR(pattern_tick(pattern), TAG, "failed to update pattern");

    // update the note only if it's actually audible (velocity > 0)
    if (track->active_step.note != pattern->state.note && pattern->state.velocity > 0) {
        ret = CALLBACK_INVOKE(&track->config.callbacks, event,
            TRACK_NOTE_CHANGE, track, &pattern->state.note);
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to invoke note change callback");
    }

    // update the velocity
    if (track->active_step.velocity != pattern->state.velocity) {
        ret = CALLBACK_INVOKE(&track->config.callbacks, event,
            TRACK_VELOCITY_CHANGE, track, &pattern->state.velocity);
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to invoke velocity change callback");
    }

    return ESP_OK;
}

esp_err_t track_set_active_pattern(track_t *track, int pattern_id) {
    ESP_RETURN_ON_FALSE(pattern_id < TRACK_MAX_PATTERNS, ESP_ERR_INVALID_ARG,
        TAG, "invalid pattern id %d", pattern_id);

    // pattern already active
    if (track->active_pattern == pattern_id) return ESP_OK;

    // if the pattern was not previously active, seek it to the current playhead position
    if (track->active_pattern != pattern_id && pattern_id != -1) {
        pattern_t *pattern = &track->patterns[pattern_id];
        ESP_RETURN_ON_ERROR(pattern_seek(pattern, track->playhead),
            TAG, "failed to initialize pattern %d", pattern_id);
    }

    // set the pattern id
    track->active_pattern = pattern_id;

    return ESP_OK;
}

pattern_t *track_get_active_pattern(track_t *track) {
    if (track->active_pattern == -1) return NULL;

    return &track->patterns[track->active_pattern];
}
