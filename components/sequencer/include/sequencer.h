#pragma once

#include <esp_err.h>
#include <esp_timer.h>
#include "track.h"
#include "callback.h"


#define SEQUENCER_NUM_TRACKS 4

#define SEQUENCER_DEFAULT_CONFIG() ((sequencer_config_t) { \
    .bpm = 120 \
})


typedef enum {
    SEQUENCER_TICK,
    SEQUENCER_PLAY,
    SEQUENCER_PAUSE,
    SEQUENCER_SEEK,
    SEQUENCER_TRACK_EVENT
} sequencer_event_t;

typedef struct {
    track_t *track;
    track_event_t event;
    void *data;
} sequencer_track_event_t;

typedef struct sequencer_t sequencer_t;
CALLBACK_DECLARE(sequencer_event, esp_err_t,
    sequencer_event_t event, sequencer_t *sequencer, void *data);


typedef struct {
    struct {
        void *context;
        CALLBACK_TYPE(sequencer_event) event;
    } callbacks;
    uint16_t bpm;
} sequencer_config_t;

struct sequencer_t {
    sequencer_config_t config;
    esp_timer_handle_t timer;

    track_t tracks[SEQUENCER_NUM_TRACKS];
    uint32_t playhead;
    bool playing;
};


esp_err_t sequencer_init(sequencer_t *sequencer, const sequencer_config_t *config);

esp_err_t sequencer_seek(sequencer_t *sequencer, uint32_t playhead);
esp_err_t sequencer_play(sequencer_t *sequencer);
esp_err_t sequencer_pause(sequencer_t *sequencer);

esp_err_t sequencer_set_bpm(sequencer_t *sequencer, uint16_t bpm);
uint64_t sequencer_get_tick_period_us(sequencer_t *sequencer);
