#pragma once

#include <esp_err.h>
#include <esp_timer.h>

#define SEQUENCER_INITIAL_TICK_SPEED_US 100000


typedef struct {
    esp_timer_handle_t timer;
} sequencer_t;


esp_err_t sequencer_create(sequencer_t *sequencer);
