#include "controller.h"
#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "usb.h"


static const char *TAG = "controller";


bool controller_supported(const controller_class_t *class, const usb_device_desc_t *desc) {
    return class->supported(desc);
}

controller_t *controller_create_from_desc(const controller_class_t *classes[], const usb_device_desc_t *desc, const controller_config_t *config) {
    // instantiate the first supported class
    for (int i = 0; classes[i] != NULL; i++) {
        if (controller_supported(classes[i], desc)) {
            return controller_create(classes[i], config);
        }
    }

    // device not supported
    ESP_LOGE(TAG, "device not supported");
    return NULL;
}

controller_t *controller_create(const controller_class_t *class,
        const controller_config_t *config) {
    controller_t *controller = malloc(class->size);
    if (controller == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for controller");
        return NULL;
    }

    controller->class = class;
    controller->config = *config;

    esp_err_t ret = class->init(controller);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize controller");
        free(controller);
        return NULL;
    }

    return controller;
}

esp_err_t controller_free(controller_t *controller) {
    ESP_RETURN_ON_ERROR(controller->class->free(controller),
        TAG,
        "Failed to free controller");

    free(controller);
    return ESP_OK;
}

esp_err_t controller_midi_recv(controller_t *controller, const midi_message_t *message) {
    return controller->class->midi_recv(controller, message);
}

esp_err_t controller_sequencer_event(controller_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    return controller->class->sequencer_event(controller, event_base, event_id, event_data);
}
