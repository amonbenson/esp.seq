#include "sequencer.h"
#include <esp_check.h>
#include "sequencer_config.h"


static const char *TAG = "sequencer";


static void sequencer_tick(void *arg) {
    sequencer_t *sequencer = (sequencer_t *) arg;
    esp_err_t err;

    // update each track
    for (int i = 0; i < SEQUENCER_NUM_TRACKS; i++) {
        err = track_tick(&sequencer->tracks[i], sequencer->playhead);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "failed to update track %d", i);
        }
    }

    // update the playhead
    sequencer->playhead++;
}

esp_err_t sequencer_init(sequencer_t *sequencer, const sequencer_config_t *config) {
    sequencer->config = *config;
    sequencer->playhead = 0;

    // initialize all tracks
    const track_config_t track_config = TRACK_DEFAULT_CONFIG();
    for (uint8_t i = 0; i < SEQUENCER_NUM_TRACKS; i++) {
        ESP_RETURN_ON_ERROR(track_init(&sequencer->tracks[i], &track_config),
            TAG, "failed to initialize track %d", i);
    }

    // create the tick timer
    const esp_timer_create_args_t timer_config = {
        .name = "sequencer_tick",
        .callback = sequencer_tick,
        .arg = sequencer,
        .skip_unhandled_events = true
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_config, &sequencer->timer),
        TAG, "failed to create timer");

    return ESP_OK;
}

esp_err_t sequencer_seek(sequencer_t *sequencer, uint32_t playhead) {
    sequencer->playhead = playhead;

    // seek all tracks
    for (int i = 0; i < SEQUENCER_NUM_TRACKS; i++) {
        ESP_RETURN_ON_ERROR(track_seek(&sequencer->tracks[i], playhead),
            TAG, "failed to seek track %d", i);
    }

    return ESP_OK;
}

esp_err_t sequencer_play(sequencer_t *sequencer) {
    ESP_RETURN_ON_FALSE(sequencer->playing == false, ESP_ERR_INVALID_STATE,
        TAG, "sequencer is already playing");

    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(sequencer->timer, sequencer_get_tick_period_us(sequencer)),
        TAG, "failed to start timer");
    sequencer->playing = true;

    return ESP_OK;
}

esp_err_t sequencer_pause(sequencer_t *sequencer) {
    ESP_RETURN_ON_FALSE(sequencer->playing == true, ESP_ERR_INVALID_STATE,
        TAG, "sequencer is already stopped");

    ESP_RETURN_ON_ERROR(esp_timer_stop(sequencer->timer),
        TAG, "failed to stop timer");
    sequencer->playing = false;

    return ESP_OK;
}

esp_err_t sequencer_set_bpm(sequencer_t *sequencer, uint16_t bpm) {
    bool was_playing = sequencer->playing;

    // stop the sequencer
    if (was_playing) {
        ESP_RETURN_ON_ERROR(sequencer_pause(sequencer),
            TAG, "failed to pause");
    }

    // update the bpm
    sequencer->config.bpm = bpm;

    // restart the sequencer
    if (was_playing) {
        ESP_RETURN_ON_ERROR(sequencer_play(sequencer),
            TAG, "failed to play");
    }

    return ESP_OK;
}

uint64_t sequencer_get_tick_period_us(sequencer_t *sequencer) {
    return (60000000 / (SEQ_PPQN * sequencer->config.bpm));
}
