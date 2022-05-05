#include "usb_midi.h"
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include <usb/usb_host.h>
#include <string.h>
#include "midi.h"


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


static const char *TAG = "usb_midi";

const uint8_t usb_midi_message_lengths[16] = {
    4, 4, 3, 4, 4, 2, 3, 4, 4, 4, 4, 4, 3, 3, 4, 2
};


static void usb_midi_sysex_reset(usb_midi_t *usb_midi) {
    // clear the buffer by resetting the length
    usb_midi->sysex_len = 0;
}

static esp_err_t usb_midi_sysex_parse(usb_midi_t *usb_midi, uint8_t *data, size_t len) {
    esp_err_t err;
    midi_message_t message;

    for (size_t i = 0; i < len && usb_midi->sysex_len < USB_MIDI_SYSEX_MAX_LEN; i++, usb_midi->sysex_len++) {
        // store the incoming byte
        usb_midi->sysex_buffer[usb_midi->sysex_len] = data[i];

        // if we've reached the stop byte, invoke the callback
        if (data[i] == 0xf7) {
            usb_midi->sysex_len++;

            err = midi_message_decode(usb_midi->sysex_buffer, usb_midi->sysex_len, &message);
            if (err != ESP_OK) return err;

            MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, recv, &message);
            usb_midi_sysex_reset(usb_midi);
        }
    }

    return ESP_OK;
}

static esp_err_t usb_midi_handle_data_in(usb_midi_t *usb_midi, uint8_t *data, int len) {
    esp_err_t err;
    midi_message_t message;
    uint8_t required_length, cin, cn;

    /* printf("raw data in: ");
    for (int i = 0; i < len; i++) printf("%02x ", data[i]);
    printf("\n"); */

    // validate mininum length
    if (len < 2) return ESP_ERR_INVALID_SIZE;

    // get cable number and code index number
    //cn = data[0] >> 4;
    cin = data[0] & 0x0F;

    // validate the message length
    required_length = usb_midi_message_lengths[cin];
    if (len < required_length) return ESP_ERR_INVALID_SIZE;

    // handle short messages
    if (cin >= USB_MIDI_CIN_NOTE_OFF && cin <= USB_MIDI_CIN_PITCH_BEND) {
        err = midi_message_decode(data + 1, len - 1, &message);
        if (err != ESP_OK) return err;

        // cin number should correspond to the midi command
        if (cin << 4 != message.command) {
            return ESP_ERR_INVALID_ARG;
        }

        MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, recv, &message);
        return ESP_OK;
    }

    // handle special messages
    err = ESP_OK;
    switch (cin) {
        case USB_MIDI_CIN_SYSEX_START:
            err = usb_midi_sysex_parse(usb_midi, data + 1, 3);

            break;
        case USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1:
            err = usb_midi_sysex_parse(usb_midi, data + 1, 1);
            usb_midi_sysex_reset(usb_midi);

            break;
        case USB_MIDI_CIN_SYSEX_END_2:
            err = usb_midi_sysex_parse(usb_midi, data + 1, 2);
            usb_midi_sysex_reset(usb_midi);

            break;
        case USB_MIDI_CIN_SYSEX_END_3:
            err = usb_midi_sysex_parse(usb_midi, data + 1, 3);
            usb_midi_sysex_reset(usb_midi);

            break;
        default:
            ESP_LOGE(TAG, "unsupported code index number %d", cin);

            return ESP_ERR_NOT_SUPPORTED;
    }

    return err;
}

static void usb_midi_data_in_callback(usb_transfer_t *transfer) {
    usb_midi_t *usb_midi = (usb_midi_t *) transfer->context;
    esp_err_t err;

    // handle the incoming data in 4 byte packets
    for (int i = 0; i < transfer->actual_num_bytes; i += 4) {
        err = usb_midi_handle_data_in(usb_midi,
            transfer->data_buffer + i,
            MIN(transfer->actual_num_bytes - i, 4));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "data in failed with code %04x", err);
        }
    }

    // continue polling if the device hasn't been closed yet
    // usb_host_transfer_submit might return an invalid state error,
    // if the device has already disconnected, we can ignore that
    if (usb_midi->device) {
        usb_host_transfer_submit(usb_midi->data_in);
    }
}

static void usb_midi_data_out_callback(usb_transfer_t *transfer) {
    //usb_midi_t *usb_midi = (usb_midi_t *) transfer->context;
    ESP_LOGI(TAG, "data out callback");
}

static esp_err_t usb_midi_open_device(usb_midi_t *usb_midi, uint8_t address) {
    esp_err_t err;
    usb_device_info_t info;
    const usb_config_desc_t *descriptor;
    const usb_standard_desc_t *d;
    int offset = 0;

    const usb_intf_desc_t *interface = NULL;
    const usb_ep_desc_t *data_in = NULL;
    const usb_ep_desc_t *data_out = NULL;

    // open the device
    usb_midi->device_address = address;
    ESP_LOGI(TAG, "opening device at 0x%02x", usb_midi->device_address);
    err = usb_host_device_open(usb_midi->client, usb_midi->device_address, &usb_midi->device);
    if (err != ESP_OK) return err;

    // get the device info
    ESP_LOGI(TAG, "getting device information");
    err = usb_host_device_info(usb_midi->device, &info);
    if (err != ESP_OK) return err;

    // get the device descriptor
    ESP_LOGI(TAG, "getting device descriptor");
    err = usb_host_get_device_descriptor(usb_midi->device, &usb_midi->device_descriptor);
    if (err != ESP_OK) return err;

    // get the configuration descriptor
    assert(usb_midi->device != NULL);
    ESP_LOGI(TAG, "getting configuration descriptor");
    usb_host_get_active_config_descriptor(usb_midi->device, &descriptor);
    if (err != ESP_OK) return err;

    // scan the configuration descriptor for interfaces and endpoints
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

    // check if we've got all the necessary descriptors
    if (!interface || !data_in || !data_out) {
        ESP_LOGE(TAG, "not a valid MIDI device");
        return ESP_FAIL;
    }
    
    // claim the interface
    ESP_LOGI(TAG, "claiming interface");
    err = usb_host_interface_claim(usb_midi->client,
        usb_midi->device,
        interface->bInterfaceNumber,
        interface->bAlternateSetting);
    if (err != ESP_OK) return err;

    // store the interface and create transfers for each endpoint
    usb_midi->interface = interface;

    size_t data_in_size = USB_MIDI_PACKET_MAX_LEN;
    err = usb_host_transfer_alloc(data_in_size, 0, &usb_midi->data_in);
    if (err != ESP_OK) return err;

    usb_midi->data_in->device_handle = usb_midi->device;
    usb_midi->data_in->bEndpointAddress = data_in->bEndpointAddress;
    usb_midi->data_in->callback = usb_midi_data_in_callback;
    usb_midi->data_in->context = (void *) usb_midi;
    usb_midi->data_in->num_bytes = data_in_size;
    
    size_t data_out_size = USB_MIDI_PACKET_MAX_LEN;
    err = usb_host_transfer_alloc(data_out_size, 0, &usb_midi->data_out);
    if (err != ESP_OK) return err;

    usb_midi->data_out->device_handle = usb_midi->device;
    usb_midi->data_out->bEndpointAddress = data_out->bEndpointAddress;
    usb_midi->data_out->callback = usb_midi_data_out_callback;
    usb_midi->data_out->context = (void *) usb_midi;

    // reset the sysex length
    usb_midi->sysex_len = 0;

    // invoke the connected callback
    MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, connected, usb_midi->device_descriptor);

    // start input polling
    err = usb_host_transfer_submit(usb_midi->data_in);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "midi device initialized (data in size: %d, data out size: %d)", data_in_size, data_out_size);

    return ESP_OK;
}

static void usb_midi_close_device(usb_midi_t *usb_midi) {
    assert(usb_midi->device != NULL);

    // invoke the disconnected callback if the interface was actually claimed
    if (usb_midi->interface) {
        MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, disconnected, usb_midi->device_descriptor);
    }

    // free the endpoints and transfers
    if (usb_midi->data_in) {
        ESP_ERROR_CHECK(usb_host_endpoint_halt(usb_midi->device, usb_midi->data_in->bEndpointAddress));
        ESP_ERROR_CHECK(usb_host_endpoint_flush(usb_midi->device, usb_midi->data_in->bEndpointAddress));
        ESP_ERROR_CHECK(usb_host_transfer_free(usb_midi->data_in));
        usb_midi->data_in = NULL;
    }
    if (usb_midi->data_out) {
        ESP_ERROR_CHECK(usb_host_endpoint_halt(usb_midi->device, usb_midi->data_out->bEndpointAddress));
        ESP_ERROR_CHECK(usb_host_endpoint_flush(usb_midi->device, usb_midi->data_out->bEndpointAddress));
        ESP_ERROR_CHECK(usb_host_transfer_free(usb_midi->data_out));
        usb_midi->data_out = NULL;
    }

    // release the interface
    if (usb_midi->interface) {
        ESP_LOGI(TAG, "releasing interface");
        ESP_ERROR_CHECK(usb_host_interface_release(usb_midi->client, usb_midi->device, usb_midi->interface->bInterfaceNumber));
        usb_midi->interface = NULL;
    }

    ESP_LOGI(TAG, "closing device 0x%02x", usb_midi->device_address);
    ESP_ERROR_CHECK(usb_host_device_close(usb_midi->client, usb_midi->device));

    usb_midi->device = NULL;
    usb_midi->device_address = 0;
}

static void usb_midi_client_event_callback(const usb_host_client_event_msg_t *msg, void *arg) {
    usb_midi_t *usb_midi = (usb_midi_t *) arg;

    switch (msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            if (usb_midi->device_address != 0) break;

            // open new device
            usb_midi_open_device(usb_midi, msg->new_dev.address);
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            if (usb_midi->device == NULL) break;

            // close device
            usb_midi_close_device(usb_midi);
            break;
        default:
            ESP_LOGE(TAG, "unknown client event %d", msg->event);
            break;
    }
}

void usb_midi_driver_task(void *arg) {
    usb_midi_t *usb_midi = (usb_midi_t *) arg;

    // create the client usb_midi
    ESP_LOGI(TAG, "registering client");
    usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = usb_midi_client_event_callback,
            .callback_arg = (void *) usb_midi
        }
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &usb_midi->client));

    // main event loop
    while (1) {
        usb_host_client_handle_events(usb_midi->client, portMAX_DELAY);
    }

    // cleanup
    ESP_LOGI(TAG, "deregistering client");
    ESP_ERROR_CHECK(usb_host_client_deregister(usb_midi->client));

    vTaskDelete(NULL);
}

esp_err_t usb_midi_init(const usb_midi_config_t *config, usb_midi_t *usb_midi) {
    memset(usb_midi, 0, sizeof(usb_midi_t));

    // store the config
    usb_midi->config = *config;

    // link the task function and argument
    usb_midi->driver_config.task = usb_midi_driver_task;
    usb_midi->driver_config.arg = (void *) usb_midi;

    return ESP_OK;
}

esp_err_t usb_midi_send(usb_midi_t *usb_midi, const midi_message_t *message) {
    esp_err_t err;
    if (!MIDI_COMMAND_IS_CHANNEL_VOICE(message->command)) {
        ESP_LOGE(TAG, "only channel voice messages are supported atm");
        return ESP_ERR_NOT_SUPPORTED;
    }

    // init the output buffer
    usb_midi->data_out->num_bytes = 4;
    usb_midi->data_out->timeout_ms = 10000;
    memset(usb_midi->data_out->data_buffer, 0, 4);

    // set the cin
    usb_midi->data_out->data_buffer[0] = message->command >> 4;

    // encode the message
    err = midi_message_encode(message, usb_midi->data_out->data_buffer + 1, 3);
    if (err != ESP_OK) return err;

    // submit the transfer
    return usb_host_transfer_submit(usb_midi->data_out);
}
