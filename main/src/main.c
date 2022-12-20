#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_check.h>

#include <usb.h>
#include <usb_midi.h>
#include <store.h>
#include <output.h>
#include <sequencer.h>

#include <controller.h>
#include <controllers/launchpad.h>
#include <controllers/generic.h>


static const char *TAG = "espmidi";


#define OUTPUT_COLUMNS 1
#define OUTPUT_ROWS 2


static const output_port_config_t output_port_configs[] = {
    { .type = OUTPUT_ANALOG, .pin = 2, .vmax_mv = 5000 /* 4840 */ },
    { .type = OUTPUT_ANALOG, .pin = 3, .vmax_mv = 4840 },
};

static const controller_class_t *controller_classes[] = {
    &controller_class_launchpad,
    &controller_class_generic,
    NULL
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
uint16_t bpm = 120;


/* uint8_t testseq_notes[] = {
    12,
    24,
    36,
    24,
};
uint8_t testseq_vels[] = {
    127,
    127,
    127,
    127,
    127,
    127
};
uint16_t bpm = 30;*/


static usb_midi_t usb_midi;
static output_t output;
static sequencer_t sequencer;

static controller_t *controller = NULL;


esp_err_t sequencer_event_callback(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data) {
    esp_err_t ret;
    
    // set the output voltage based on note and velocity events
    switch (event) {
        case SEQUENCER_TRACK_EVENT:;
            sequencer_track_event_t *track_event = (sequencer_track_event_t *) data;
            switch (track_event->event) {
                case TRACK_NOTE_CHANGE:;
                    uint8_t note = *(uint8_t *) track_event->data;
                    ret = output_set_voltage(&output, 0, 0, OUTPUT_NOTE_TO_VOLTAGE(note));
                    ESP_RETURN_ON_ERROR(ret, TAG, "failed to set output voltage");
                    break;
                case TRACK_VELOCITY_CHANGE:;
                    uint8_t velocity = *(uint8_t *) track_event->data;
                    ret = output_set_voltage(&output, 0, 1, OUTPUT_VELOCITY_TO_VOLTAGE(velocity));
                    ESP_RETURN_ON_ERROR(ret, TAG, "failed to set output voltage");
                    break;
            }
            break;
        default:
            break;
    }

    // forward sequencer events to the current controller
    if (controller != NULL) {
        ret = controller_sequencer_event(controller, event, sequencer, data);
        ESP_RETURN_ON_ERROR(ret, TAG, "failed to forward sequencer event to controller");
    }

    return ESP_OK;
}

esp_err_t controller_midi_send_callback(void *context, controller_t *controller, const midi_message_t *message) {
    return usb_midi_send(&usb_midi, message);
}

void usb_midi_connected_callback(const usb_device_desc_t *desc) {
    const controller_config_t config = {
        .callbacks = {
            .midi_send = controller_midi_send_callback
        },
        .sequencer = &sequencer,
        .output = &output
    };

    // create the usb controller
    controller = controller_create_from_desc(controller_classes, desc, &config);
    if (controller == NULL) {
        ESP_LOGE(TAG, "failed to create controller");
        return;
    }
}

void usb_midi_disconnected_callback(const usb_device_desc_t *desc) {
    // destroy the usb controller
    controller_free(controller);
    controller = NULL;
}

void usb_midi_recv_callback(const midi_message_t *message) {
    // usb midi --> controller
    if (controller != NULL) {
        controller_midi_recv(controller, message);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP MIDI v2.0");

    // setup the file store
    //ESP_ERROR_CHECK(store_init());

    // setup usb midi interface
    const usb_midi_config_t usb_midi_config = {
        .callbacks = {
            .connected = usb_midi_connected_callback,
            .disconnected = usb_midi_disconnected_callback,
            .recv = usb_midi_recv_callback
        }
    };
    ESP_ERROR_CHECK(usb_midi_init(&usb_midi_config, &usb_midi));
    ESP_ERROR_CHECK(usb_init(&usb_midi.driver_config));

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
        .callbacks = {
            .event = sequencer_event_callback,
        },
        .bpm = bpm
    };
    ESP_ERROR_CHECK(sequencer_init(&sequencer, &sequencer_config));

    // setup a test sequence
    track_t *track = &sequencer.tracks[0];
    //ESP_ERROR_CHECK(track_set_active_pattern(track, 0));

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
