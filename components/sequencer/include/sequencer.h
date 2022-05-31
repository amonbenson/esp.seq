#pragma once

#include <esp_err.h>
#include <esp_timer.h>
#include <esp_event.h>
#include "track.h"


#define SEQUENCER_NUM_TRACKS 4

#define SEQUENCER_DEFAULT_CONFIG() ((sequencer_config_t) { \
    .bpm = 120 \
})


ESP_EVENT_DECLARE_BASE(SEQUENCER_EVENT);
enum {
    SEQUENCER_TICK_EVENT,
    SEQUENCER_PLAY_EVENT,
    SEQUENCER_PAUSE_EVENT,
    SEQUENCER_SEEK_EVENT
};


typedef struct {
    esp_event_handler_t event_handler;
    void *event_handler_arg;

    uint16_t bpm;
} sequencer_config_t;

typedef struct {
    sequencer_config_t config;
    esp_timer_handle_t timer;
    esp_event_loop_handle_t event_loop;

    track_t tracks[SEQUENCER_NUM_TRACKS];
    uint32_t playhead;
    bool playing;
} sequencer_t;


esp_err_t sequencer_init(sequencer_t *sequencer, const sequencer_config_t *config);

esp_err_t sequencer_seek(sequencer_t *sequencer, uint32_t playhead);
esp_err_t sequencer_play(sequencer_t *sequencer);
esp_err_t sequencer_pause(sequencer_t *sequencer);

esp_err_t sequencer_set_bpm(sequencer_t *sequencer, uint16_t bpm);
uint64_t sequencer_get_tick_period_us(sequencer_t *sequencer);
