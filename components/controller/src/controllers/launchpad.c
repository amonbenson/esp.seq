#include "controllers/launchpad.h"
#include <esp_log.h>
#include <esp_check.h>
#include <string.h>
#include "controllers/launchpad_types.h"


static const char *TAG = "launchpad controller";

// static sysex messages
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

static esp_err_t controller_launchpad_send_lpui(controller_launchpad_t *controller) {
    return controller_midi_send_sysex(&controller->super,
        controller->ui.buffer,
        controller->ui.buffer_size);
}

esp_err_t controller_launchpad_init(void *context) {
    esp_err_t ret;
    controller_launchpad_t *controller = context;

    // setup launchpad ui
    ret = lpui_init(&controller->ui);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize launchpad ui");

    // select programmer layout in standalone mode
    ret = controller_midi_send_sysex(&controller->super, lp_sysex_standalone_mode, sizeof(lp_sysex_standalone_mode));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set standalone mode");

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_prog_layout, sizeof(lp_sysex_prog_layout));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set programmer layout");

    // clear all leds
    ret = lp_clear(context);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear launchpad");

    // initialize pattern editor
    controller->pattern_editor = (lpui_pattern_editor_t) {
        .cmp = {
            .pos = { x: 1, y: 5 },
            .size = { width: 8, height: 4 },
        },
        .page = 0,
        .track_id = 0,
        .pattern = NULL,
        .step_position = 0
    };

    // initialize piano editor
    controller->piano_editor = (lpui_piano_editor_t) {
        .cmp = {
            .pos = { x: 1, y: 1 },
            .size = { width: 8, height: 4 }
        }
    };

    return ESP_OK;
}

esp_err_t controller_launchpad_free(void *context) {
    return ESP_OK;
}

esp_err_t controller_launchpad_note_event(controller_launchpad_t *controller, uint8_t velocity, uint8_t note) {
    esp_err_t ret;

    /*
    uint8_t *b = lp_sysex_start(0x0B);

    uint8_t x = note % 10;
    uint8_t y = note / 10;
    b = lp_sysex_led_color(b, x, y, velocity > 0 ? LP_COLOR_SEQ_ACTIVE : (lp_color_t) { 0 });

    ret = lp_sysex_commit(controller, b);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to draw sequencer");
    */

    return ESP_OK;
}

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message) {
    controller_launchpad_t *controller = context;

    switch (message->command) {
        case MIDI_COMMAND_NOTE_ON:
            return controller_launchpad_note_event(controller,
                message->note_on.velocity, message->note_on.note);
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

    // get the active pattern
    track_t *track = &sequencer->tracks[controller->pattern_editor.track_id];
    pattern_t *pattern = track_get_active_pattern(track);
    controller->pattern_editor.pattern = pattern;

    switch (event) {
        case SEQUENCER_TICK:
            lpui_pattern_editor_set_pattern(&controller->pattern_editor, pattern);

            // TODO: this should be moved somewhere central
            if (controller->pattern_editor.cmp.redraw_required) {
                lpui_pattern_editor_draw(&controller->ui, &controller->pattern_editor);
                controller_launchpad_send_lpui(controller);

                lpui_piano_editor_draw(&controller->ui, &controller->piano_editor);
                controller_launchpad_send_lpui(controller);
            }

            break;
        default:
            break;
    }
    
    return ESP_OK;
}
