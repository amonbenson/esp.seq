#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <output.h>
#include <sequencer.h>


static const char *TAG = "espmidi";


static const output_pins_t output_digital_pins = {
    .length = 2,
    .pins = { 4, 5 }
};
static const output_pins_t output_analog_pins = {
    .length = 2,
    .pins = { 2, 3 }
};


static output_t output;
static sequencer_t sequencer;


void sequencer_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    uint32_t playhead;
    pattern_atomic_step_t *step;

    if (event_base == SEQUENCER_EVENT) {
        switch (event_id) {
            case SEQUENCER_TICK_EVENT:
                playhead = *(uint32_t *) event_data;
                //ESP_LOGI(TAG, "tick %d", playhead);
                break;
            case SEQUENCER_PLAY_EVENT:
                ESP_LOGI(TAG, "play");
                break;
            case SEQUENCER_PAUSE_EVENT:
                ESP_LOGI(TAG, "pause");
                break;
            case SEQUENCER_SEEK_EVENT:
                playhead = *(uint32_t *) event_data;
                ESP_LOGI(TAG, "seek %d", playhead);
                break;
            default:
                ESP_LOGE(TAG, "unknown sequencer event: %d", event_id);
                break;
        }
    }

    if (event_base == TRACK_EVENT) {
        switch (event_id) {
            case TRACK_NOTE_ON_EVENT:
                step = (pattern_atomic_step_t *) event_data;
                ESP_LOGI(TAG, "note on %d %d", step->note, step->velocity);
                break;
            case TRACK_NOTE_OFF_EVENT:
                step = (pattern_atomic_step_t *) event_data;
                ESP_LOGI(TAG, "note off %d", step->note);
                break;
            default:
                ESP_LOGE(TAG, "unknown track event: %d", event_id);
                break;
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP MIDI");

    // setup the output unit
    const output_config_t output_config = {
        .digital_pins = &output_digital_pins,
        .analog_pins = &output_analog_pins
    };
    output_init(&output, &output_config);
    
    // setup the sequencer
    const sequencer_config_t sequencer_config = {
        .bpm = 120,
        .event_handler = sequencer_event_handler,
        .event_handler_arg = NULL
    };
    sequencer_init(&sequencer, &sequencer_config);

    // test sequence
    track_t *track = &sequencer.tracks[0];
    ESP_ERROR_CHECK(track_set_active_pattern(track, 0));

    pattern_t *pattern = track_get_active_pattern(track);
    for (uint16_t i = 0; i < pattern->config.step_length; i++) {
        pattern->steps[i].state.note = 60 + i;
        pattern->steps[i].state.velocity = 127;
    }

    ESP_ERROR_CHECK(sequencer_play(&sequencer));

    vTaskDelete(NULL);
}
