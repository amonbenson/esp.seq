#include "usbmidi_driver.h"
#include <freertos/task.h>
#include <esp_log.h>


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


static const char *TAG = "usbmidi_driver";

usbmidi_callbacks_t usbmidi_callbacks = { 0 };


static uint32_t usbmidi_driver_add_event(usbmidi_driver_t *driver, usbmidi_event_t event) {
    return xQueueSend(driver->events, &event, 0);
}

static void usbmidi_driver_handle_data_in(usbmidi_driver_t *driver, uint8_t *data, int len) {
    if (len < 1) return;

    uint8_t cn = data[0] >> 4;
    uint8_t cin = data[0] & 0x0F;

    uint8_t *message = data + 1;
    int message_len = len - 1;

    switch (cin) {
        case USBMIDI_CIN_MISC:
            ESP_LOGE(TAG, "misc messages unimplemented");
            break;
        case USBMIDI_CIN_CABLE_EVENT:
            ESP_LOGE(TAG, "cable event messages unimplemented");
            break;
        case USBMIDI_CIN_SYSCOM_2:
            if (message_len != USBMIDI_CIN_SYSCOM_2_LEN) return;
            ESP_LOGE(TAG, "syscom 2 messages unimplemented");
            break;
        case USBMIDI_CIN_SYSCOM_3:
            if (message_len != USBMIDI_CIN_SYSCOM_3_LEN) return;
            ESP_LOGE(TAG, "syscom 3 messages unimplemented");
            break;
        case USBMIDI_CIN_SYSEX_START:
            if (message_len != USBMIDI_CIN_SYSEX_START_LEN) return;
            ESP_LOGE(TAG, "sysex start messages unimplemented");
            break;
        case USBMIDI_CIN_SYSEX_END_1_SYSCOM_1:
            if (message_len != USBMIDI_CIN_SYSEX_END_1_SYSCOM_1_LEN) return;
            ESP_LOGE(TAG, "sysex end 1 syscom 1 messages unimplemented");
            break;
        case USBMIDI_CIN_SYSEX_END_2:
            if (message_len != USBMIDI_CIN_SYSEX_END_2_LEN) return;
            ESP_LOGE(TAG, "sysex end 2 messages unimplemented");
            break;
        case USBMIDI_CIN_SYSEX_END_3:
            if (message_len != USBMIDI_CIN_SYSEX_END_3_LEN) return;
            ESP_LOGE(TAG, "sysex end 3 messages unimplemented");
            break;
        case USBMIDI_CIN_NOTE_OFF:
            if (message_len != USBMIDI_CIN_NOTE_OFF_LEN) return;
            if (usbmidi_callbacks.note_off) {
                usbmidi_callbacks.note_off(message[0] & 0x0f, message[1], message[2]);
            }
            break;
        case USBMIDI_CIN_NOTE_ON:
            if (message_len != USBMIDI_CIN_NOTE_ON_LEN) return;
            if (usbmidi_callbacks.note_on) {
                usbmidi_callbacks.note_on(message[0] & 0x0f, message[1], message[2]);
            }
            break;
        case USBMIDI_CIN_POLY_KEY_PRESSURE:
            if (message_len != USBMIDI_CIN_POLY_KEY_PRESSURE_LEN) return;
            ESP_LOGE(TAG, "poly key pressure messages unimplemented");
            break;
        case USBMIDI_CIN_CONTROL_CHANGE:
            if (message_len != USBMIDI_CIN_CONTROL_CHANGE_LEN) return;
            ESP_LOGE(TAG, "control change messages unimplemented");
            break;
        case USBMIDI_CIN_PROGRAM_CHANGE:
            if (message_len != USBMIDI_CIN_PROGRAM_CHANGE_LEN) return;
            ESP_LOGE(TAG, "program change messages unimplemented");
            break;
        case USBMIDI_CIN_CHANNEL_PRESSURE:
            if (message_len != USBMIDI_CIN_CHANNEL_PRESSURE_LEN) return;
            ESP_LOGE(TAG, "channel pressure messages unimplemented");
            break;
        case USBMIDI_CIN_PITCH_BEND:
            if (message_len != USBMIDI_CIN_PITCH_BEND_LEN) return;
            ESP_LOGE(TAG, "pitch bend messages unimplemented");
            break;
        case USBMIDI_CIN_BYTE:
            if (message_len != USBMIDI_CIN_BYTE_LEN) return;
            ESP_LOGE(TAG, "single byte messages unimplemented");
            break;
        default:
            ESP_LOGE(TAG, "unknown code index number %d", cin);
            break;
    }
}

static void usbmidi_driver_data_in_callback(usb_transfer_t *transfer) {
    usbmidi_driver_t *driver = (usbmidi_driver_t *) transfer->context;

    // handle the incoming data
    usbmidi_driver_handle_data_in(driver, transfer->data_buffer, transfer->actual_num_bytes);

    // continue polling
    if (driver->data_in) {
        ESP_ERROR_CHECK(usb_host_transfer_submit(driver->data_in));
    }
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

                // only bulk transfer is supported
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

    size_t data_in_size = MIN(USB_EP_DESC_GET_MPS(data_in), USBMIDI_PACKET_SIZE);
    ESP_ERROR_CHECK(usb_host_transfer_alloc(data_in_size, 0, &driver->data_in));
    driver->data_in->device_handle = driver->device;
    driver->data_in->bEndpointAddress = data_in->bEndpointAddress;
    driver->data_in->callback = usbmidi_driver_data_in_callback;
    driver->data_in->context = (void *) driver;
    driver->data_in->num_bytes = data_in_size;
    
    size_t data_out_size = MIN(USB_EP_DESC_GET_MPS(data_out), USBMIDI_PACKET_SIZE);
    ESP_ERROR_CHECK(usb_host_transfer_alloc(data_out_size, 0, &driver->data_out));
    driver->data_out->device_handle = driver->device;
    driver->data_out->bEndpointAddress = data_out->bEndpointAddress;
    driver->data_out->callback = usbmidi_driver_data_out_callback;
    driver->data_out->context = (void *) driver;

    // start input polling
    ESP_ERROR_CHECK(usb_host_transfer_submit(driver->data_in));

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
