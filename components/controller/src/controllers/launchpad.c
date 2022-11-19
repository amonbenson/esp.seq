#include "controllers/launchpad.h"
#include <esp_log.h>


static const char *TAG = "launchpad controller";

const controller_class_t controller_class_launchpad = {
    .size = sizeof(controller_launchpad_t),
    .supported = (controller_supported_function) controller_launchpad_supported,
    .init = (controller_init_function) controller_launchpad_init,
    .free = (controller_free_function) controller_launchpad_free,
    .midi_recv = (controller_midi_recv_function) controller_launchpad_midi_recv,
    .sequencer_event = (controller_sequencer_event_function) controller_launchpad_sequencer_event
};


bool controller_launchpad_supported(const usb_device_desc_t *desc) {
    // check the vendor and product id
    return desc->idVendor == LAUNCHPAD_VENDOR_ID && desc->idProduct == LAUNCHPAD_PRO_PRODUCT_ID;
}

esp_err_t controller_launchpad_init(controller_launchpad_t *controller) {
    return ESP_OK;
}

esp_err_t controller_launchpad_free(controller_launchpad_t *controller) {
    return ESP_OK;
}

esp_err_t controller_launchpad_midi_recv(controller_launchpad_t *controller, const midi_message_t *message) {
    return ESP_OK;
}

esp_err_t controller_launchpad_sequencer_event(controller_launchpad_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    return ESP_OK;
}
