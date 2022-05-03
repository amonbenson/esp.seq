#include "usb.h"
#include <freertos/task.h>
#include <esp_log.h>


static const char *TAG = "usb";


static void usb_daemon_task() {
    bool has_clients = true;
    bool has_devices = true;
    uint32_t event_flags;

    // handle daemon events
    ESP_LOGI(TAG, "starting daemon");
    while (has_clients || has_devices) {
        ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));

        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            has_clients = false;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            has_devices = false;
        }
    }

    // cleanup
    ESP_LOGI(TAG, "no more clients or devices");
    ESP_ERROR_CHECK(usb_host_uninstall());

    vTaskDelete(NULL);
}

esp_err_t usb_init(const usb_driver_config_t *driver) {
    esp_err_t err;
    TaskHandle_t daemon_task;
    TaskHandle_t driver_task;

    // init usb
    ESP_LOGI(TAG, "initializing usb host stack");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };
    err = usb_host_install(&host_config);
    if (err != ESP_OK) return err;

    // start the daemon and driver tasks
    xTaskCreatePinnedToCore(usb_daemon_task, "usb_daemon", 4096, NULL, 2, &daemon_task, 0);
    xTaskCreatePinnedToCore(driver->task, "usb_driver", 4096, (void *) driver->arg, 3, &driver_task, 0);

    return ESP_OK;
}
