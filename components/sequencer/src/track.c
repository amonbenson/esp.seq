#include "track.h"
#include <esp_check.h>


static const char *TAG = "sequencer: track";

ESP_EVENT_DEFINE_BASE(TRACK_EVENT);


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

static void track_pattern_step_change_callback(void *arg, pattern_atomic_step_t step) {
    track_t *track = (track_t *) arg;
    esp_err_t ret;

    // nothing to update (consecutive steps with the same note and velocity)
    if (!track->config.sequencer_event_loop) return;

    // update the note only if it's actually audible (velocity > 0)
    if (track->active_step.note != step.note && step.velocity > 0) {
        track_note_change_event_t event = {
            .track = track,
            .note = step.note
        };
        ret = esp_event_post_to(track->config.sequencer_event_loop,
            TRACK_EVENT,
            TRACK_NOTE_CHANGE_EVENT,
            &event,
            sizeof(event),
            portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "failed to post note change event: %s", esp_err_to_name(ret));
        }
        track->active_step.note = step.note;
    }

    // update the velocity
    if (track->active_step.velocity != step.velocity) {
        track_velocity_change_event_t event = {
            .track = track,
            .velocity = step.velocity
        };
        ret = esp_event_post_to(track->config.sequencer_event_loop,
            TRACK_EVENT,
            TRACK_VELOCITY_CHANGE_EVENT,
            &event,
            sizeof(event),
            portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "failed to post velocity change event: %s", esp_err_to_name(ret));
        }
        track->active_step.velocity = step.velocity;
    }
}

esp_err_t track_tick(track_t *track, uint32_t playhead) {
    pattern_t *pattern = track_get_active_pattern(track);
    if (pattern == NULL) return ESP_OK;

    return pattern_tick(pattern, track_pattern_step_change_callback, track);
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
