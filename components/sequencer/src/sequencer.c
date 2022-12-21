#include "sequencer.h"
#include <esp_check.h>
#include "sequencer_config.h"


static const char *TAG = "sequencer";


static esp_err_t sequencer_tick(sequencer_t *sequencer) {
    esp_err_t ret;

    // update each track
    for (int i = 0; i < SEQUENCER_NUM_TRACKS; i++) {
        ret = track_tick(&sequencer->tracks[i], sequencer->playhead);
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to update track %d", i);
    }

    // update the playhead position
    sequencer->playhead++;

    ret = CALLBACK_INVOKE(&sequencer->config.callbacks, event,
        SEQUENCER_TICK,
        sequencer,
        &sequencer->playhead);
    ESP_RETURN_ON_ERROR(ret, TAG, "failed to invoke tick callback");

    return ESP_OK;
}

static void sequencer_timer_callback(void *arg) {
    sequencer_t *sequencer = (sequencer_t *) arg;

    // update the sequencer
    esp_err_t err = sequencer_tick(sequencer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to tick sequencer: %s", esp_err_to_name(err));
    }
}

static esp_err_t sequencer_track_event_callback(void *context, track_event_t event, track_t *track, void *data) {
    sequencer_t *sequencer = (sequencer_t *) context;
    sequencer_track_event_t sequencer_data = {
        .track = track,
        .event = event,
        .data = data,
    };

    // pass the callback on to the sequencer handler, but leave a reference
    // to the track that triggered the event
    return CALLBACK_INVOKE(&sequencer->config.callbacks, event,
        SEQUENCER_TRACK_EVENT,
        sequencer,
        &sequencer_data);
}

esp_err_t sequencer_init(sequencer_t *sequencer, const sequencer_config_t *config) {
    esp_err_t ret;

    sequencer->config = *config;
    sequencer->playhead = 0;

    // initialize all tracks.
    const track_config_t track_config = {
        .callbacks = {
            .context = sequencer,
            .event = sequencer_track_event_callback
        }
    };
    for (uint8_t i = 0; i < SEQUENCER_NUM_TRACKS; i++) {
        ret = track_init(&sequencer->tracks[i], &track_config);
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to initialize track %d", i);
    }

    // create the tick timer
    const esp_timer_create_args_t timer_config = {
        .name = "sequencer_tick",
        .callback = sequencer_timer_callback,
        .arg = sequencer,
        .skip_unhandled_events = true
    };
    ret = esp_timer_create(&timer_config, &sequencer->timer);
    ESP_RETURN_ON_ERROR(ret, TAG, "failed to create timer");

    return ESP_OK;
}

esp_err_t sequencer_seek(sequencer_t *sequencer, uint32_t playhead) {
    esp_err_t ret;

    sequencer->playhead = playhead;

    // seek all tracks
    for (int i = 0; i < SEQUENCER_NUM_TRACKS; i++) {
        ret = track_seek(&sequencer->tracks[i], playhead);
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to seek track %d", i);
    }

    ret = CALLBACK_INVOKE(&sequencer->config.callbacks, event,
        SEQUENCER_SEEK,
        sequencer,
        &playhead);
    ESP_RETURN_ON_ERROR(ret, TAG, "failed to invoke seek callback");

    return ESP_OK;
}

esp_err_t sequencer_play(sequencer_t *sequencer) {
    esp_err_t ret;

    if (sequencer->playing) return ESP_OK;

    ret = esp_timer_start_periodic(sequencer->timer, sequencer_get_tick_period_us(sequencer));
    ESP_RETURN_ON_ERROR(ret, TAG, "failed to start timer");
    sequencer->playing = true;

    ret = CALLBACK_INVOKE(&sequencer->config.callbacks, event,
        SEQUENCER_PLAY,
        sequencer,
        NULL);
    ESP_RETURN_ON_ERROR(ret, TAG, "failed to invoke play callback");

    return ESP_OK;
}

esp_err_t sequencer_pause(sequencer_t *sequencer) {
    esp_err_t ret;

    if (!sequencer->playing) return ESP_OK;

    ret = esp_timer_stop(sequencer->timer);
    ESP_RETURN_ON_ERROR(ret, TAG, "failed to stop timer");
    sequencer->playing = false;

    ret = CALLBACK_INVOKE(&sequencer->config.callbacks, event,
        SEQUENCER_PAUSE,
        sequencer,
        NULL);
    ESP_RETURN_ON_ERROR(ret, TAG, "failed to invoke pause callback");

    return ESP_OK;
}

esp_err_t sequencer_set_bpm(sequencer_t *sequencer, uint16_t bpm) {
    esp_err_t ret;

    sequencer->config.bpm = bpm;

    // restart the timer
    if (sequencer->playing) {
        ret = esp_timer_stop(sequencer->timer);
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to stop timer");

        ret = esp_timer_start_periodic(sequencer->timer, sequencer_get_tick_period_us(sequencer));
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to start timer");
    }

    return ESP_OK;
}

uint64_t sequencer_get_tick_period_us(sequencer_t *sequencer) {
    return (60000000 / (SEQ_PPQN * sequencer->config.bpm));
}

pattern_t *sequencer_get_active_pattern(sequencer_t *sequencer, int track_id) {
    if (track_id < 0 || track_id >= SEQUENCER_NUM_TRACKS) return NULL;

    track_t *track = &sequencer->tracks[track_id];
    return track_get_active_pattern(track);
}
