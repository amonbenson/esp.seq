#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <output.h>
#include <sequencer.h>


static const char *TAG = "espmidi";


#define OUTPUT_MATRIX_COLUMNS 1
#define OUTPUT_MATRIX_ROWS 2

#define MILLIVOLTS_PER_SEMITONE 120 // 12 semitones = 1 octave = 1 volt
#define ROOTNOTE 36 // C2


static const gpio_num_t output_digital_pins[] = { 3 };
static const gpio_num_t output_analog_pins[] = { 2 };


static output_t output;
static sequencer_t sequencer;


uint32_t note_to_millivolts(uint32_t note) {
    // lower limit
    if (note < ROOTNOTE) return 0;

    uint32_t millivolts = (note - ROOTNOTE) * MILLIVOLTS_PER_SEMITONE;

    // upper limit
    if (millivolts > OUTPUT_ANALOG_MILLIVOLTS) return OUTPUT_ANALOG_MILLIVOLTS;

    return millivolts;
}

uint32_t velocity_to_millivolts(uint32_t velocity) {
    return velocity * OUTPUT_ANALOG_MILLIVOLTS / 127;
}


void sequencer_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == SEQUENCER_EVENT) {
        switch (event_id) {
            case SEQUENCER_TICK_EVENT:;
                uint32_t playhead = *(uint32_t *) event_data;
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
            case TRACK_NOTE_CHANGE_EVENT:;
                uint8_t note = *(uint8_t *) event_data;
                ESP_LOGI(TAG, "note: %d", note);
                output_set(&output, 0, 0, note_to_millivolts(note));
                break;
            case TRACK_VELOCITY_CHANGE_EVENT:;
                uint8_t velocity = *(uint8_t *) event_data;
                ESP_LOGI(TAG, "velocity: %d", velocity);
                output_set(&output, 0, 1, velocity_to_millivolts(velocity));
                break;
            default:
                ESP_LOGE(TAG, "unknown track event: %d", event_id);
                break;
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP MIDI v2.0");

    // setup the output unit
    const output_config_t output_config = {
        .digital_pins = output_digital_pins,
        .num_digital_pins = sizeof(output_digital_pins) / sizeof(gpio_num_t),
        .analog_pins = output_analog_pins,
        .num_analog_pins = sizeof(output_analog_pins) / sizeof(gpio_num_t),
        .matrix_size = {
            .columns = OUTPUT_MATRIX_COLUMNS,
            .rows = OUTPUT_MATRIX_ROWS
        }
    };
    ESP_ERROR_CHECK(output_init(&output, &output_config));
    
    // setup the sequencer
    const sequencer_config_t sequencer_config = {
        .bpm = 15,
        .event_handler = sequencer_event_handler,
        .event_handler_arg = NULL
    };
    ESP_ERROR_CHECK(sequencer_init(&sequencer, &sequencer_config));

    // test sequence
    track_t *track = &sequencer.tracks[0];
    ESP_ERROR_CHECK(track_set_active_pattern(track, 0));

    pattern_t *pattern = track_get_active_pattern(track);
    for (int i = 0; i < pattern->config.step_length; i++) {
        pattern_step_t *step = &pattern->steps[i];
        step->atomic.note = 60 + i;
        step->atomic.velocity = 127;
        step->gate = 64;
        step->probability = 64;
    }

    ESP_ERROR_CHECK(sequencer_play(&sequencer));

    vTaskDelete(NULL);
}
