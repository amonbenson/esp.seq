#include "controllers/launchpad/launchpad.h"
#include <esp_log.h>
#include <esp_check.h>
#include <string.h>


static const char *TAG = "launchpad controller";

// static sysex messages
static const uint8_t lp_sysex_header[] = { LP_SYSEX_HEADER };
static const uint8_t lp_sysex_standalone_mode[] = { LP_SYSEX_HEADER, 0x21, 0x01, 0xF7 };
static const uint8_t lp_sysex_prog_layout[] = { LP_SYSEX_HEADER, 0x2C, 0x03, 0xF7 };
static const uint8_t lp_sysex_clear_grid[] = {
    LP_SYSEX_HEADER,
    LP_SYSEX_COMMAND_SET_LEDS_ALL,
    LP_SYSEX_COLOR_CLEAR,
    0xF7
};
static const uint8_t lp_sysex_clear_side_led[] = {
    LP_SYSEX_HEADER,
    LP_SYSEX_COMMAND_SET_LEDS,
    LP_SYSEX_SIDE_LED,
    LP_SYSEX_COLOR_CLEAR,
    0xF7
};

// buffer for dynamic sysex messages
uint8_t lp_sysex_buffer[LP_SYSEX_BUFFER_SIZE];


const controller_class_t controller_class_launchpad = {
    .size = sizeof(controller_launchpad_t),
    .functions = {
        .supported = controller_launchpad_supported,
        .init = controller_launchpad_init,
        .free = controller_launchpad_free,
        .midi_recv = controller_launchpad_midi_recv,
        .sequencer_event = controller_launchpad_sequencer_event
    }
};


bool controller_launchpad_supported(void *context, const usb_device_desc_t *desc) {
    // check the vendor and product id
    return desc->idVendor == LP_VENDOR_ID && desc->idProduct == LP_PRODUCT_ID;
}

static esp_err_t lp_clear(controller_launchpad_t *controller) {
    esp_err_t ret;

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_clear_grid, sizeof(lp_sysex_clear_grid));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear grid");

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_clear_side_led, sizeof(lp_sysex_clear_side_led));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear side led");

    return ESP_OK;
}

static uint8_t *lp_sysex_start(uint8_t command) {
    uint8_t *b = lp_sysex_buffer + sizeof(lp_sysex_header);

    *b++ = command;
    return b;
}

static uint8_t *lp_sysex_color(uint8_t *b, const lp_color_t color) {
    *b++ = color.red;
    *b++ = color.green;
    *b++ = color.blue;
    return b;
}

static uint8_t *lp_sysex_led_color(uint8_t *b, uint8_t x, uint8_t y, const lp_color_t color) {
    *b++ = x + y * 10;
    b = lp_sysex_color(b, color);
    return b;
}

static esp_err_t lp_sysex_commit(controller_launchpad_t *controller, uint8_t *b) {
    *b++ = 0xF7;

    return controller_midi_send_sysex(&controller->super,
        lp_sysex_buffer, b - lp_sysex_buffer);
}

static esp_err_t pattern_editor_draw(controller_launchpad_t *controller, pattern_t *pattern) {
    esp_err_t ret;

    lp_pattern_editor_t *editor = &controller->pattern_editor;
    lp_bounds_t *bounds = &editor->bounds;
    uint8_t page_steps = bounds->width * bounds->height;
    uint8_t step_offset = editor->page * page_steps;

    uint8_t *b = lp_sysex_start(0x0B);

    for (uint8_t y = 0; y < bounds->height; y++) {
        for (uint8_t x = 0; x < bounds->width; x++) {
            uint8_t step_index = y * bounds->width + x + step_offset;
            pattern_step_t *step = &pattern->steps[step_index];

            lp_color_t color;
            if (step_index == pattern->step_position) {
                color = LP_COLOR_SEQ_ACTIVE;
            } else if (step->atomic.velocity > 0) {
                color = LP_COLOR_SEQ_ENABLED;
            } else {
                color = LP_COLOR_SEQ_BG;
            }

            b = lp_sysex_led_color(b, bounds->x + x, 9 - bounds->y - y, color);
        }
    }

    ret = lp_sysex_commit(controller, b);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to draw sequencer");

    return ESP_OK;
}

static esp_err_t piano_editor_draw(controller_launchpad_t *controller, pattern_t *pattern) {
    esp_err_t ret;

    return ESP_OK;
}

esp_err_t controller_launchpad_init(void *context) {
    esp_err_t ret;
    controller_launchpad_t *controller = context;

    // setup sysex buffer
    memcpy(lp_sysex_buffer, lp_sysex_header, sizeof(lp_sysex_header));

    // select programmer layout in standalone mode
    ret = controller_midi_send_sysex(&controller->super, lp_sysex_standalone_mode, sizeof(lp_sysex_standalone_mode));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set standalone mode");

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_prog_layout, sizeof(lp_sysex_prog_layout));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set programmer layout");

    // clear all leds
    ret = lp_clear(context);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear launchpad");

    // initialize pattern editor
    controller->pattern_editor = (lp_pattern_editor_t) {
        .pattern = NULL,
        .bounds = {
            .x = 1,
            .y = 1,
            .width = 8,
            .height = 4
        },
        .page = 0,
        .last_step_position = 0
    };

    // initialize piano editor

    return ESP_OK;
}

esp_err_t controller_launchpad_free(void *context) {
    return ESP_OK;
}

esp_err_t controller_launchpad_note_event(controller_launchpad_t *controller, uint8_t velocity, uint8_t note) {
    esp_err_t ret;

    uint8_t *b = lp_sysex_start(0x0B);

    uint8_t x = note % 10;
    uint8_t y = note / 10;
    b = lp_sysex_led_color(b, x, y, velocity > 0 ? LP_COLOR_SEQ_ACTIVE : (lp_color_t) { 0 });

    ret = lp_sysex_commit(controller, b);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to draw sequencer");

    return ESP_OK;
}

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message) {
    controller_launchpad_t *controller = context;

    switch (message->command) {
        case MIDI_COMMAND_NOTE_ON:
            if (message->note_on.velocity > 0) {
                return controller_launchpad_note_event(controller,
                    message->note_on.velocity, message->note_on.note);
            } else {
                return controller_launchpad_note_event(controller,
                    0, message->note_on.note);
            }
            break;
        case MIDI_COMMAND_NOTE_OFF:
            return controller_launchpad_note_event(controller,
                0, message->note_off.note);
        default:
            break;
    }
    
    return ESP_OK;
}

esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data) {
    controller_launchpad_t *controller = context;

    track_t *track = &sequencer->tracks[0];
    pattern_t *pattern = track_get_active_pattern(track);
    controller->pattern_editor.pattern = pattern;

    switch (event) {
        case SEQUENCER_TICK:
            if (controller->pattern_editor.last_step_position != pattern->step_position) {
                // automatic scrolling
                uint8_t page_steps = controller->pattern_editor.bounds.width * controller->pattern_editor.bounds.height;
                controller->pattern_editor.page = pattern->step_position / page_steps;

                // redraw the pattern editor
                pattern_editor_draw(controller, pattern);

                controller->pattern_editor.last_step_position = pattern->step_position;
            }
            break;
        default:
            break;
    }
    
    return ESP_OK;
}
