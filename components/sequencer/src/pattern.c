#include "pattern.h"
#include <esp_check.h>
#include <string.h>
#include "sequencer_utils.h"


static const char *TAG = "sequencer: pattern";

static int pattern_id_counter = 0;


static int pattern_get_next_id() {
    return pattern_id_counter++;
}


void pattern_step_init(pattern_step_t *step) {
    step->atomic.note = 60;
    step->atomic.velocity = 0;
    step->gate = 64;
    step->probability = 127;
}

esp_err_t pattern_init(pattern_t *pattern, const pattern_config_t *config) {
    pattern->config = *config;
    pattern->id = pattern_get_next_id();
    pattern_seek(pattern, 0);

    // allocate memory for all steps
    pattern->steps = malloc(pattern->config.step_length * sizeof(pattern_step_t));
    ESP_RETURN_ON_FALSE(pattern->steps, ESP_ERR_NO_MEM, TAG, "failed to allocate pattern steps");

    // initialize all steps
    for (int i = 0; i < pattern->config.step_length; i++) {
        pattern_step_init(&pattern->steps[i]);
    }

    return ESP_OK;
}

esp_err_t pattern_resize(pattern_t *pattern, uint16_t step_length) {
    // resize the pattern steps array
    pattern->steps = realloc(pattern->steps, step_length * sizeof(pattern_step_t));
    ESP_RETURN_ON_FALSE(pattern->steps, ESP_ERR_NO_MEM, TAG, "failed to reallocate pattern steps");

    // initialize any new steps
    for (int i = pattern->config.step_length; i < step_length; i++) {
        pattern_step_init(&pattern->steps[i]);
    }

    // seek to the beginning if the position is now out of bounds
    if (pattern->step_position >= step_length) {
        pattern_seek(pattern, 0);
    }

    // set the new length
    pattern->config.step_length = step_length;

    return ESP_OK;
}

esp_err_t pattern_seek(pattern_t *pattern, uint32_t playhead) {
    pattern->substep_position = playhead % pattern->config.resolution;
    pattern->step_position = (playhead / pattern->config.resolution) % pattern->config.step_length;

    return ESP_OK;
}

esp_err_t pattern_tick(pattern_t *pattern) {
    pattern_step_t *step = pattern_get_active_step(pattern);

    // start playing a step
    if (pattern->substep_position == 0) {
        // decide if the step should be enabled and when it should stop playing
        pattern->active_step_enabled = step->probability == 127 || seq_rand() % 128 < step->probability;
        pattern->active_step_off = 1 + pattern->substep_position + step->gate * (pattern->config.resolution - 1) / 127;

        if (pattern->active_step_enabled) {
            // set the pattern state
            pattern->state = step->atomic;
        }
    }

    // stop playing a step (if gate = 128, this will never be called)
    if (pattern->substep_position == pattern->active_step_off && pattern->active_step_enabled) {
        pattern->state = (pattern_atomic_step_t) { 0 };
    }

    // move to the next substep
    pattern->substep_position += 1;
    if (pattern->substep_position >= pattern->config.resolution) {
        pattern->substep_position = 0;

        // move to the next step
        pattern->step_position += 1;
        if (pattern->step_position >= pattern->config.step_length) {
            pattern->step_position = 0;
        }
    }

    return ESP_OK;
}

pattern_step_t *pattern_get_active_step(pattern_t *pattern) {
    return &pattern->steps[pattern->step_position];
}

pattern_step_t *pattern_get_previous_step(pattern_t *pattern) {
    uint16_t position;

    // get the position of the previous step
    if (pattern->step_position == 0) position = pattern->config.step_length - 1;
    else position = pattern->step_position - 1;

    // return that step
    return &pattern->steps[position];
}

pattern_step_t *pattern_get_next_step(pattern_t *pattern) {
    uint16_t position;

    // get the position of the next step
    if (pattern->step_position == pattern->config.step_length - 1) position = 0;
    else position = pattern->step_position + 1;

    // return that step
    return &pattern->steps[position];
}

inline uint32_t pattern_step_to_ticks(pattern_t *pattern, uint16_t step) {
    return step * pattern->config.resolution;
}

inline uint16_t pattern_ticks_to_step(pattern_t *pattern, uint32_t ticks) {
    return ticks / pattern->config.resolution;
}
