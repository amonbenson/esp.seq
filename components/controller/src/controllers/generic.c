#include "controllers/generic.h"
#include <esp_log.h>


static const char *TAG = "controller_generic";

const controller_class_t controller_class_generic = {
    .size = sizeof(controller_generic_t),
    .init = controller_generic_init,
    .free = controller_generic_free,
};


esp_err_t controller_generic_init(controller_t *controller) {
    controller_generic_t *generic = (controller_generic_t *) controller;
    ESP_LOGI(TAG, "Initializing generic controller");

    return ESP_OK;
}

esp_err_t controller_generic_free(controller_t *controller) {
    controller_generic_t *generic = (controller_generic_t *) controller;
    ESP_LOGI(TAG, "Freeing generic controller");

    return ESP_OK;
}
