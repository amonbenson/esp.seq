#include "usbmidi_driver.h"
#include <freertos/task.h>
#include <esp_log.h>


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


static const char *TAG = "usbmidi_driver";


static uint32_t usbmidi_driver_add_event(usbmidi_driver_t *driver, usbmidi_event_t event) {
    return xQueueSend(driver->events, &event, 0);
}

static void usbmidi_driver_data_in_callback(usb_transfer_t *transfer) {
    usbmidi_driver_t *driver = (usbmidi_driver_t *) transfer->context;
    ESP_LOGI(TAG, "data in callback");
}

static void usbmidi_driver_data_out_callback(usb_transfer_t *transfer) {
    usbmidi_driver_t *driver = (usbmidi_driver_t *) transfer->context;
    ESP_LOGI(TAG, "data out callback");
}

static void usbmidi_driver_open_dev(usbmidi_driver_t *driver) {
    assert(driver->device_address != 0);
    ESP_LOGI(TAG, "opening device at 0x%02x", driver->device_address);
    ESP_ERROR_CHECK(usb_host_device_open(driver->client, driver->device_address, &driver->device));

    // get the device info next
    usbmidi_driver_add_event(driver, USBMIDI_EVENT_GET_DEV_INFO);
}

static void usbmidi_driver_get_dev_info(usbmidi_driver_t *driver) {
    usb_device_info_t info;

    assert(driver->device != NULL);
    ESP_LOGI(TAG, "getting device information");
    ESP_ERROR_CHECK(usb_host_device_info(driver->device, &info));

    // get the device descriptor next
    usbmidi_driver_add_event(driver, USBMIDI_EVENT_GET_DEV_DESC);
}

static void usbmidi_driver_get_dev_desc(usbmidi_driver_t *driver) {
    assert(driver->device != NULL);
    ESP_LOGI(TAG, "getting device descriptor");
    ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver->device, &driver->device_descriptor));

    // get the configuration descriptor next
    usbmidi_driver_add_event(driver, USBMIDI_EVENT_GET_CONFIG_DESC);
}

static void usbmidi_driver_get_config_desc(usbmidi_driver_t *driver) {
    const usb_config_desc_t *descriptor;
    const usb_standard_desc_t *d;
    int offset = 0;

    const usb_intf_desc_t *interface = NULL;
    const usb_ep_desc_t *data_in = NULL;
    const usb_ep_desc_t *data_out = NULL;

    assert(driver->device != NULL);
    ESP_LOGI(TAG, "getting configuration descriptor");
    ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver->device, &descriptor));

    // scan all descriptors
    for (d = (usb_standard_desc_t *) descriptor; d != NULL; d = usb_parse_next_descriptor(d, descriptor->wTotalLength, &offset)) {
        switch (d->bDescriptorType) {
            case USB_W_VALUE_DT_INTERFACE:;
                const usb_intf_desc_t *i = (const usb_intf_desc_t *) d;

                // validate the device class
                if (i->bInterfaceClass != USB_CLASS_AUDIO) break;
                if (i->bInterfaceSubClass != USB_SUBCLASS_MIDISTREAMING) break;

                // mark this device as a midi device and reset the endpoints
                interface = i;
                data_in = NULL;
                data_out = NULL;

                break;
            case USB_W_VALUE_DT_ENDPOINT:;
                const usb_ep_desc_t *e = (const usb_ep_desc_t *) d;

                // make sure bulk transfer is supported
                if (!(e->bmAttributes & USB_TRANSFER_TYPE_BULK)) break;

                // store the endpoints
                if (e->bEndpointAddress & 0x80) {
                    data_in = e;
                } else {
                    data_out = e;
                }

                break;
            default:
                break;
        }
    }

    // check if we've got all the descriptors
    if (!interface || !data_in || !data_out) {
        ESP_LOGE(TAG, "not a valid MIDI device");
        return;
    }
    
    // claim the interface
    ESP_LOGI(TAG, "claiming interface");
    ESP_ERROR_CHECK(usb_host_interface_claim(driver->client,
        driver->device,
        interface->bInterfaceNumber,
        interface->bAlternateSetting));

    // store the interface and create transfers for each endpoint
    driver->interface = interface;

    size_t data_in_size = MIN(data_in->wMaxPacketSize, USBMIDI_PACKET_SIZE);
    ESP_ERROR_CHECK(usb_host_transfer_alloc(data_in_size, 0, &driver->data_in));
    driver->data_in->callback = usbmidi_driver_data_in_callback;
    driver->data_in->context = (void *) driver;
    
    size_t data_out_size = MIN(data_out->wMaxPacketSize, USBMIDI_PACKET_SIZE);
    ESP_ERROR_CHECK(usb_host_transfer_alloc(data_out_size, 0, &driver->data_out));
    driver->data_out->callback = usbmidi_driver_data_out_callback;
    driver->data_out->context = (void *) driver;

    ESP_LOGI(TAG, "midi device initialized (data in size: %d, data out size: %d)", data_in_size, data_out_size);
}

static void usbmidi_driver_get_str_desc(usbmidi_driver_t *driver) {
    usb_device_info_t info;

    assert(driver->device != NULL);
    ESP_LOGI(TAG, "getting string descriptors");
    ESP_ERROR_CHECK(usb_host_device_info(driver->device, &info));

    if (info.str_desc_manufacturer) {
        usb_print_string_descriptor(info.str_desc_manufacturer);
    }

    if (info.str_desc_product) {
        usb_print_string_descriptor(info.str_desc_product);
    }

    if (info.str_desc_serial_num) {
        usb_print_string_descriptor(info.str_desc_serial_num);
    }
}

static void usbmidi_driver_close_dev(usbmidi_driver_t *driver) {
    assert(driver->device != NULL);

    // free the transfers
    if (driver->data_in) {
        usb_host_transfer_free(driver->data_in);
        driver->data_in = NULL;
    }
    if (driver->data_out) {
        usb_host_transfer_free(driver->data_out);
        driver->data_out = NULL;
    }

    // release the interface
    if (driver->interface) {
        ESP_LOGI(TAG, "releasing interface");
        usb_host_interface_release(driver->client, driver->device, driver->interface->bInterfaceNumber);
        driver->interface = NULL;
    }

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
            case USBMIDI_EVENT_GET_DEV_INFO:
                usbmidi_driver_get_dev_info(&driver);
                break;
            case USBMIDI_EVENT_GET_DEV_DESC:
                usbmidi_driver_get_dev_desc(&driver);
                break;
            case USBMIDI_EVENT_GET_CONFIG_DESC:
                usbmidi_driver_get_config_desc(&driver);
                break;
            case USBMIDI_EVENT_GET_STR_DESC:
                usbmidi_driver_get_str_desc(&driver);
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
