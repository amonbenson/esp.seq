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
    0x00,
    0xF7
};
static const uint8_t lp_sysex_clear_side_led[] = {
    LP_SYSEX_HEADER,
    LP_SYSEX_COMMAND_SET_LEDS_RGB,
    LP_SYSEX_SIDE_LED,
    0x00,
    0x00,
    0x00,
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

// private ui callback functions
static esp_err_t _lpui_sysex_ready(void *context, lpui_t *ui, uint8_t *buffer, size_t length) {
    controller_launchpad_t *controller = context;

    // pass the sysex data to the midi send callback
    return controller_midi_send_sysex(&controller->super, buffer, length);
}

static esp_err_t _play_button_pressed(void *context, button_t *button) {
    controller_launchpad_t *controller = context;

    // start the sequencer
    return sequencer_play(controller->super.config.sequencer);
}

static esp_err_t _play_button_released(void *context, button_t *button) {
    controller_launchpad_t *controller = context;

    // stop the sequencer
    return sequencer_pause(controller->super.config.sequencer);
}

static esp_err_t _pattern_editor_step_selected(void *context, pattern_editor_t *editor, uint16_t step_position) {
    return controller_launchpad_select_step(context, step_position);
}

esp_err_t controller_launchpad_init(void *context) {
    esp_err_t ret;
    controller_launchpad_t *controller = context;

    controller->selected_track_id = -1;
    controller->selected_step_position = -1;

    // setup launchpad ui
    const lpui_config_t ui_config = {
        .callbacks = {
            .context = controller,
            .sysex_ready = _lpui_sysex_ready
        }
    };
    ret = lpui_init(&controller->ui, &ui_config);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize launchpad ui");

    // select programmer layout in standalone mode
    ret = controller_midi_send_sysex(&controller->super, lp_sysex_standalone_mode, sizeof(lp_sysex_standalone_mode));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set standalone mode");

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_prog_layout, sizeof(lp_sysex_prog_layout));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set programmer layout");

    // clear all leds
    ret = lp_clear(context);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear launchpad");

    // initialize the play and record buttons
    const button_config_t play_button_config = {
        .cmp_config = {
            .pos = { x: 0, y: 2 }
        },
        .callbacks = {
            .context = controller,
            .pressed = _play_button_pressed,
            .released = _play_button_released
        },
        .color = LPUI_COLOR_GREEN,
        .mode = BUTTON_MODE_TOGGLE
    };
    button_init(&controller->play_button, &play_button_config);
    lpui_add_component(&controller->ui, &controller->play_button.cmp);
    button_draw(&controller->play_button);

    // initialize pattern editor
    const pattern_editor_config_t pe_config = (pattern_editor_config_t) {
        .callbacks = {
            .context = controller,
            .step_selected = _pattern_editor_step_selected,
        },
        .cmp_config = {
            .pos = { x: 1, y: 5 },
            .size = { width: 8, height: 4 }
        }
    };
    pattern_editor_init(&controller->pattern_editor, &pe_config);
    lpui_add_component(&controller->ui, &controller->pattern_editor.cmp);

    // initialize piano editor
    /* controller->piano_editor = (lpui_piano_editor_t) {
        .cmp = {
            .config = {
                .pos = { x: 1, y: 1 },
                .size = { width: 8, height: 4 },
            }
        }
    };
    lpui_add_component(&controller->ui, &controller->piano_editor.cmp); */

    // select the first track
    controller_launchpad_select_track(controller, 0);

    return ESP_OK;
}

esp_err_t controller_launchpad_free(void *context) {
    return ESP_OK;
}

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message) {
    controller_launchpad_t *controller = context;
    lpui_t *ui = &controller->ui;

    // let the launchpad ui handle the event
    lpui_midi_recv(ui, message);
    
    return ESP_OK;
}

esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data) {
    controller_launchpad_t *controller = context;

    switch (event) {
        case SEQUENCER_TICK:
            // if playing, update the pattern editor position
            pattern_editor_update_step_position(&controller->pattern_editor);
            break;
        default:
            break;
    }
    
    return ESP_OK;
}

esp_err_t controller_launchpad_select_track(void *context, int track_id) {
    controller_launchpad_t *controller = context;

    // set the new selected track id
    if (controller->selected_track_id == track_id) return ESP_OK;
    controller->selected_track_id = track_id;

    // select the active pattern on that track (or NULL if no track is selected)
    pattern_t *pattern = sequencer_get_active_pattern(controller->super.config.sequencer, track_id);
    ESP_RETURN_ON_ERROR(pattern_editor_set_pattern(&controller->pattern_editor, track_id, pattern),
        TAG, "Failed to set pattern editor pattern");

    return ESP_OK;
}

esp_err_t controller_launchpad_select_step(void *context, uint16_t step_position) {
    controller_launchpad_t *controller = context;
    pattern_t *pattern = controller->pattern_editor.pattern;

    // TODO: only do this if we are in a specific step edit mode
    /* if (pattern && controller->super.config.sequencer->playing) {
        // toggle the step velocity
        pattern_step_t *step = &pattern->steps[step_position];
        if (step->atomic.velocity > 0) {
            step->atomic.velocity = 0;
        } else {
            step->atomic.velocity = 127;
        }

        // redraw that specific step
        ESP_RETURN_ON_ERROR(pattern_editor_draw_steps(&controller->pattern_editor, &step_position, 1),
            TAG, "Failed to redraw pattern editor step");
    } */

    // set the new selected step position
    if (controller->selected_step_position == step_position) return ESP_OK;
    controller->selected_step_position = step_position;

    return ESP_OK;
}
