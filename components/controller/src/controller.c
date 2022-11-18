#include "controller.h"
#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"


static const char *TAG = "controller";


controller_t *controller_create(const controller_class_t *class) {
    esp_err_t err;

    controller_t *controller = malloc(class->size);
    if (controller == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for controller");
        return NULL;
    }

    controller->class = class;
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
