#include "usbmidi_driver.h"
#include <freertos/task.h>
#include <esp_log.h>


static const char *TAG = "usbmidi_driver";


static void usbmidi_driver_open_dev(usbmidi_driver_t *driver) {
    assert(driver->client != NULL);
    ESP_LOGI(TAG, "opening device 0x%02x", driver->device_address);
    ESP_ERROR_CHECK(usb_host_device_open(driver->client, driver->device_address, &driver->device));

    // get the device's information
    driver->actions &= ~USBMIDI_ACTION_OPEN_DEV;
    // TODO
}

static void usbmidi_driver_close_dev(usbmidi_driver_t *driver) {
    ESP_LOGI(TAG, "closing device 0x%02x", driver->device_address);
    ESP_ERROR_CHECK(usb_host_device_close(driver->client, driver->device));

    driver->device = NULL;
    driver->device_address = 0;

    driver->actions &= ~USBMIDI_ACTION_CLOSE_DEV;
}

static void usbmidi_midi_client_event_callback(const usb_host_client_event_msg_t *msg, void *arg) {
    usbmidi_driver_t *driver = (usbmidi_driver_t *) arg;

    switch (msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            // open new device
            if (driver->device_address == 0) {
                driver->device_address = msg->new_dev.address;
                driver->actions |= USBMIDI_ACTION_OPEN_DEV;
            }
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            // close device
            if (driver->device != NULL) {
                driver->actions |= USBMIDI_ACTION_CLOSE_DEV;
            }
            break;
        default:
            abort();
            break;
    }
}

void usbmidi_driver_task(void *arg) {
    usbmidi_driver_t driver = { 0 };

    // create the client driver
    ESP_LOGI(TAG, "registering client");
    usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = usbmidi_midi_client_event_callback,
            .callback_arg = (void *) &driver
        }
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &driver.client));

    // handle driver events
    while (1) {
        if (driver.actions == 0) {
            usb_host_client_handle_events(driver.client, portMAX_DELAY);
        }

        if (driver.actions & USBMIDI_ACTION_OPEN_DEV) {
            usbmidi_driver_open_dev(&driver);
        }
        if (driver.actions & USBMIDI_ACTION_CLOSE_DEV) {
            usbmidi_driver_close_dev(&driver);
        }
    }

    // cleanup
    ESP_LOGI(TAG, "deregistering client");
    ESP_ERROR_CHECK(usb_host_client_deregister(driver.client));

    vTaskDelete(NULL);
}
