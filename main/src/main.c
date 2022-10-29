#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <store.h>
#include <output.h>
#include <sequencer.h>


static const char *TAG = "espmidi";


#define OUTPUT_COLUMNS 1
#define OUTPUT_ROWS 2


static const output_port_config_t output_port_configs[] = {
    { .type = OUTPUT_ANALOG, .pin = 2, .vmax_mv = 4840 },
    { .type = OUTPUT_ANALOG, .pin = 3, .vmax_mv = 4840 },
};

uint8_t testseq_notes[] = {
    12,
    12,
    10,
    10,
    12,
    12,
    10,
    10,
    12,
    12,
    0,
    0,
    15,
    15,
    0,
    0,
    12,
    12,
    10,
    10,
    12,
    12,
    10,
    10,
    12,
    12,
    0,
    0,
    0,
    0,
    0,
    0
};
uint8_t testseq_vels[] = {
    127,
    64,
    127,
    64,
    127,
    64,
    127,
    64,
    127,
    127,
    0,
    0,
    127,
    127,
    0,
    0,
    127,
    64,
    127,
    64,
    127,
    64,
    127,
    64,
    127,
    64,
    64,
    127,
    127,
    127,
    0,
    0
};


static output_t output;
static sequencer_t sequencer;


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
                //ESP_LOGI(TAG, "note: %d", note);
                output_set_voltage(&output, 0, 0, note * 1000 / 12);
                break;
            case TRACK_VELOCITY_CHANGE_EVENT:;
                uint8_t velocity = *(uint8_t *) event_data;
                //ESP_LOGI(TAG, "velocity: %d", velocity);
                output_set_voltage(&output, 0, 1, velocity * 5000 / 127);
                break;
            default:
                ESP_LOGE(TAG, "unknown track event: %d", event_id);
                break;
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP MIDI v2.0");

    // setup the file store
    //ESP_ERROR_CHECK(store_init());

    // setup the output unit
    uint8_t num_port_configs = sizeof(output_port_configs) / sizeof(output_port_configs[0]);
    uint8_t num_ports = OUTPUT_COLUMNS * OUTPUT_ROWS;
    if (num_port_configs != num_ports) {
        ESP_LOGE(TAG, "invalid output configuration: %d ports != %d cols x %d rows",
            num_port_configs, OUTPUT_COLUMNS, OUTPUT_ROWS);
        return;
    }

    const output_config_t output_config = {
        .num_columns = OUTPUT_COLUMNS,
        .num_rows = OUTPUT_ROWS,
        .port_configs = output_port_configs
    };
    ESP_ERROR_CHECK(output_init(&output, &output_config));
    
    // setup the sequencer
    const sequencer_config_t sequencer_config = {
        .bpm = 120,
        .event_handler = sequencer_event_handler,
        .event_handler_arg = NULL
    };
    ESP_ERROR_CHECK(sequencer_init(&sequencer, &sequencer_config));

    // setup a test sequence
    track_t *track = &sequencer.tracks[0];
    ESP_ERROR_CHECK(track_set_active_pattern(track, 0));

    pattern_t *pattern = track_get_active_pattern(track);

    uint16_t testseq_length = sizeof(testseq_notes) / sizeof(testseq_notes[0]);
    ESP_ERROR_CHECK(pattern_resize(pattern, testseq_length));

    for (int i = 0; i < pattern->config.step_length; i++) {
        pattern_step_t *step = &pattern->steps[i];
        step->atomic.note = testseq_notes[i];
        step->atomic.velocity = testseq_vels[i];
        step->gate = 64;
        step->probability = 127;
    }

    // start the sequencer
    ESP_ERROR_CHECK(sequencer_play(&sequencer));

    vTaskDelete(NULL);
}
