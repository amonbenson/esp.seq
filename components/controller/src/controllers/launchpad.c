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

static esp_err_t controller_launchpad_ui_sysex_ready(void *context, lpui_t *ui, uint8_t *buffer, size_t length) {
    controller_launchpad_t *controller = context;

    // pass the sysex to the midi send callback
    return controller_midi_send_sysex(&controller->super, buffer, length);
}

esp_err_t controller_launchpad_init(void *context) {
    esp_err_t ret;
    controller_launchpad_t *controller = context;

    // setup launchpad ui
    const lpui_config_t ui_config = {
        .callbacks = {
            .context = controller,
            .sysex_ready = controller_launchpad_ui_sysex_ready
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

    // initialize pattern editor
    const pattern_editor_config_t pe_config = (pattern_editor_config_t) {
        .cmp_config = {
            .pos = { x: 1, y: 5 },
            .size = { width: 8, height: 4 }
        },
        .sequencer = controller->super.config.sequencer
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
            // update the pattern editor
            pattern_editor_update(&controller->pattern_editor);

            break;
        default:
            break;
    }
    
    return ESP_OK;
}
