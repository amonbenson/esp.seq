#include "usbmidi_driver.h"
#include <freertos/task.h>
#include <esp_log.h>


static const char *TAG = "usbmidi_driver";


static uint32_t usbmidi_driver_add_event(usbmidi_driver_t *driver, usbmidi_event_t event) {
    return xQueueSend(driver->events, &event, 0);
}

static void usbmidi_driver_open_dev(usbmidi_driver_t *driver) {
    assert(driver->client != NULL);
    ESP_LOGI(TAG, "opening device 0x%02x", driver->device_address);
    ESP_ERROR_CHECK(usb_host_device_open(driver->client, driver->device_address, &driver->device));

    // get the device info next
    usbmidi_driver_add_event(driver, USBMIDI_EVENT_GET_DEV_INFO);
}

static void usbmidi_driver_close_dev(usbmidi_driver_t *driver) {
    ESP_LOGI(TAG, "closing device 0x%02x", driver->device_address);
    ESP_ERROR_CHECK(usb_host_device_close(driver->client, driver->device));

    driver->device = NULL;
    driver->device_address = 0;
}

static void usbmidi_midi_client_event_callback(const usb_host_client_event_msg_t *msg, void *arg) {
    usbmidi_driver_t *driver = (usbmidi_driver_t *) arg;

    switch (msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            if (driver->device_address != 0) break;

            // open new device
            driver->device_address = msg->new_dev.address;
            usbmidi_driver_add_event(driver, USBMIDI_EVENT_OPEN_DEV);
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            if (driver->device == NULL) break;

            // close device
            usbmidi_driver_add_event(driver, USBMIDI_EVENT_CLOSE_DEV);
            break;
        default:
            ESP_LOGE(TAG, "unknown client event %d", msg->event);
            break;
    }
}

void usbmidi_driver_task(void *arg) {
    usbmidi_driver_t driver = { 0 };
    driver.events = xQueueCreate(USBMIDI_EVENT_QUEUE_LEN, sizeof(usbmidi_event_t));

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
    usbmidi_event_t event;
    while (1) {
        // handle client events if there are no others
        if (!xQueueReceive(driver.events, &event, 0)) {
            usb_host_client_handle_events(driver.client, portMAX_DELAY);
            continue;    
        }

        // handle driver events
        switch (event) {
            case USBMIDI_EVENT_OPEN_DEV:
                usbmidi_driver_open_dev(&driver);
                break;
            case USBMIDI_EVENT_CLOSE_DEV:
                usbmidi_driver_close_dev(&driver);
                break;
            default:
                ESP_LOGE(TAG, "unknown driver event %d", event);
                break;
        }
    }

    // cleanup
    ESP_LOGI(TAG, "deregistering client");
    ESP_ERROR_CHECK(usb_host_client_deregister(driver.client));

    vTaskDelete(NULL);
}
