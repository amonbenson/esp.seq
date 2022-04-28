#include "sequencer.h"


static void sequencer_timer_callback(void *arg) {
    sequencer_t *sequencer = (sequencer_t *) arg;
}


esp_err_t sequencer_create(sequencer_t *sequencer) {
    esp_err_t err;

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &sequencer_timer_callback,
        .arg = sequencer,
        .name = "sequencer"
    };

    // setup and start the tick timer
    err = esp_timer_create(&periodic_timer_args, &sequencer->timer);
    if (err != ESP_OK) return err;

    err = esp_timer_start_periodic(&sequencer->timer, SEQUENCER_INITIAL_TICK_SPEED_US);
    if (err != ESP_OK) return err;

    return ESP_OK;
}
