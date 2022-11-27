#include "controllers/launchpad.h"
#include <esp_log.h>
#include <esp_check.h>


static const char *TAG = "launchpad controller";

static const uint8_t lp_sysex_standalone_mode[] = { LP_SYSEX_HEADER, 0x21, 0x01, 0xF7 };
static const uint8_t lp_sysex_prog_layout[] = { LP_SYSEX_HEADER, 0x2C, 0x03, 0xF7 };
static const uint8_t lp_sysex_clear_grid[] = { LP_SYSEX_HEADER, 0x0E, 0x00, 0xF7 };
static const uint8_t lp_sysex_clear_side_led[] = { LP_SYSEX_HEADER, 0x0A, 0x63, 0x00, 0xF7 };


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

static esp_err_t controller_launchpad_clear(controller_launchpad_t *controller) {
    esp_err_t ret;

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_clear_grid, sizeof(lp_sysex_clear_grid));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear grid");

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_clear_side_led, sizeof(lp_sysex_clear_side_led));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear side led");

    return ESP_OK;
}

esp_err_t controller_launchpad_init(void *context) {
    esp_err_t ret;
    controller_launchpad_t *controller = context;

    // select programmer layout in standalone mode
    ret = controller_midi_send_sysex(&controller->super, lp_sysex_standalone_mode, sizeof(lp_sysex_standalone_mode));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set standalone mode");

    ret = controller_midi_send_sysex(&controller->super, lp_sysex_prog_layout, sizeof(lp_sysex_prog_layout));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set programmer layout");

    // clear all leds
    ret = controller_launchpad_clear(context);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to clear launchpad");

    return ESP_OK;
}

esp_err_t controller_launchpad_free(void *context) {
    return ESP_OK;
}

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message) {
    return ESP_OK;
}

esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data) {
    return ESP_OK;
}
