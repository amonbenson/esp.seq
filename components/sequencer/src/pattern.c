#include "pattern.h"
#include <esp_check.h>
#include <string.h>


static const char *TAG = "sequencer: pattern";
static int pattern_id_counter = 0;


esp_err_t pattern_init(pattern_t *pattern, const pattern_config_t *config) {
    pattern->config = *config;
    pattern->id = pattern_id_counter++;
    pattern_seek(pattern, 0);

    // allocate memory for all steps
    pattern->steps = calloc(pattern->config.step_length, sizeof(pattern_step_t));
    ESP_RETURN_ON_FALSE(pattern->steps, ESP_ERR_NO_MEM, TAG, "failed to allocate pattern steps");

    return ESP_OK;
}

esp_err_t pattern_resize(pattern_t *pattern, uint16_t step_length) {
    // resize the pattern steps array
    pattern->steps = realloc(pattern->steps, step_length * sizeof(pattern_step_t));
    ESP_RETURN_ON_FALSE(pattern->steps, ESP_ERR_NO_MEM, TAG, "failed to reallocate pattern steps");

    // zero out the new steps
    if (step_length > pattern->config.step_length) {
        memset(&pattern->steps[pattern->config.step_length], 0, (step_length - pattern->config.step_length) * sizeof(pattern_step_t));
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

esp_err_t pattern_tick(pattern_t *pattern, pattern_step_change_callback_t callback, void *callback_arg) {
    // play the active step
    if (pattern->substep_position == 0 && callback) {
        pattern_step_t *step = pattern_get_active_step(pattern);
        callback(callback_arg, step->state);
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
