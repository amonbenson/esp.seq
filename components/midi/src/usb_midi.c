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


static void usb_midi_sysex_reset(usb_midi_t *usb_midi) {
    // clear the buffer by resetting the length
    usb_midi->in.sysex_len = 0;
}

static esp_err_t usb_midi_sysex_parse(usb_midi_t *usb_midi, const uint8_t *data, size_t len) {
    esp_err_t err;
    midi_message_t message;
    usb_midi_port_t *in = &usb_midi->in;

    for (size_t i = 0; i < len && in->sysex_len < USB_MIDI_SYSEX_BUFFER_SIZE; i++, in->sysex_len++) {
        // store the incoming byte
        in->sysex_buffer[in->sysex_len] = data[i];

        // if we've reached the stop byte, invoke the callback
        if (data[i] == 0xf7) {
            in->sysex_len++;

            err = midi_message_decode(in->sysex_buffer, in->sysex_len, &message);
            if (err != ESP_OK) return err;

            MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, recv, &message);
            usb_midi_sysex_reset(usb_midi);
        }
    }

    return ESP_OK;
}

static esp_err_t usb_midi_parse_packet(usb_midi_t *usb_midi, const usb_midi_packet_t *packet) {
    esp_err_t err;
    midi_message_t message;
    uint8_t cin;
    const uint8_t *data;

    /* printf("raw data in: ");
    for (int i = 0; i < len; i++) printf("%02x ", data[i]);
    printf("\n"); */

    // get cable number and code index number
    //cn = packet->cn_cin >> 4;
    cin = packet->cn_cin & 0x0F;
    data = packet->data;

    // handle short messages
    if (cin >= USB_MIDI_CIN_NOTE_OFF && cin <= USB_MIDI_CIN_PITCH_BEND) {
        err = midi_message_decode(data, 3, &message);
        if (err != ESP_OK) return err;

        // cin number should correspond to the midi command
        if (cin != message.command >> 4) {
            return ESP_ERR_INVALID_ARG;
        }

        MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, recv, &message);
        return ESP_OK;
    }

    // handle special messages
    err = ESP_OK;
    switch (cin) {
        case USB_MIDI_CIN_SYSEX_START:
            err = usb_midi_sysex_parse(usb_midi, data, 3);

            break;
        case USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1:
            err = usb_midi_sysex_parse(usb_midi, data, 1);
            usb_midi_sysex_reset(usb_midi);

            break;
        case USB_MIDI_CIN_SYSEX_END_2:
            err = usb_midi_sysex_parse(usb_midi, data, 2);
            usb_midi_sysex_reset(usb_midi);

            break;
        case USB_MIDI_CIN_SYSEX_END_3:
            err = usb_midi_sysex_parse(usb_midi, data, 3);
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
    const usb_midi_packet_t *packet;
    esp_err_t err;
    int i, n;

    // device is not valid anymore
    if (usb_midi->state != USB_MIDI_CONNECTED) return;

    // validate the size
    n = transfer->actual_num_bytes / 4;
    if (n == 0) {
        ESP_LOGE(TAG, "incoming packet: invalid number of bytes %d", n);
        return;
    }

    // handle the incoming data in 4 byte packets
    for (i = 0, packet = (usb_midi_packet_t *) transfer->data_buffer; i < n; i++, packet++) {
        err = usb_midi_parse_packet(usb_midi, packet);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "data in packet could not be parsed (code 0x%04x)", err);
        }
    }

    // continue polling
    usb_host_transfer_submit(usb_midi->in.transfer);
}

static void usb_midi_data_out_callback(usb_transfer_t *transfer) {
    usb_midi_t *usb_midi = (usb_midi_t *) transfer->context;

    // device is not valid anymore
    if (usb_midi->state != USB_MIDI_CONNECTED) return;

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
    usb_transfer_t *transfer;

    // make sure the device is not connected already
    assert(usb_midi->state == USB_MIDI_DISCONNECTED);

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
    usb_midi->in.endpoint = data_in;
    usb_midi->out.endpoint = data_out;

    err = usb_host_transfer_alloc(USB_MIDI_TRANSFER_MAX_SIZE, 0, &usb_midi->in.transfer);
    if (err != ESP_OK) return err;

    transfer = usb_midi->in.transfer;
    transfer->device_handle = usb_midi->device;
    transfer->bEndpointAddress = data_in->bEndpointAddress;
    transfer->callback = usb_midi_data_in_callback;
    transfer->context = (void *) usb_midi;
    transfer->num_bytes = USB_MIDI_TRANSFER_MAX_SIZE;
    
    err = usb_host_transfer_alloc(USB_MIDI_TRANSFER_MAX_SIZE, 0, &usb_midi->out.transfer);
    if (err != ESP_OK) return err;

    transfer = usb_midi->out.transfer;
    transfer->device_handle = usb_midi->device;
    transfer->bEndpointAddress = data_out->bEndpointAddress;
    transfer->callback = usb_midi_data_out_callback;
    transfer->context = (void *) usb_midi;
    transfer->num_bytes = USB_MIDI_TRANSFER_MAX_SIZE;

    // allocate memory for all data buffers
    usb_midi->in.packet_queue = xQueueCreate(USB_MIDI_PACKET_QUEUE_SIZE, sizeof(usb_midi_packet_t));
    usb_midi->in.sysex_buffer = malloc(USB_MIDI_SYSEX_BUFFER_SIZE);
    usb_midi->in.sysex_len = 0;

    usb_midi->out.packet_queue = xQueueCreate(USB_MIDI_PACKET_QUEUE_SIZE, sizeof(usb_midi_packet_t));
    usb_midi->out.sysex_buffer = malloc(USB_MIDI_SYSEX_BUFFER_SIZE);
    usb_midi->out.sysex_len = 0;

    // mark as connected and invoke the callback
    usb_midi->state = USB_MIDI_CONNECTED;
    MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, connected, usb_midi->device_descriptor);

    // start input polling
    err = usb_host_transfer_submit(usb_midi->in.transfer);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "midi device initialized");

    return ESP_OK;
}

static void usb_midi_close_device(usb_midi_t *usb_midi) {
    assert(usb_midi->state == USB_MIDI_CONNECTED);

    // mark as disconnected and invoke the callback
    usb_midi->state = USB_MIDI_DISCONNECTED;
    MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, disconnected, usb_midi->device_descriptor);

    // free the transfers
    ESP_ERROR_CHECK(usb_host_transfer_free(usb_midi->in.transfer));
    usb_midi->in.transfer = NULL;

    ESP_ERROR_CHECK(usb_host_transfer_free(usb_midi->out.transfer));
    usb_midi->out.transfer = NULL;

    // free the endpoints
    ESP_ERROR_CHECK(usb_host_endpoint_halt(usb_midi->device, usb_midi->in.endpoint->bEndpointAddress));
    ESP_ERROR_CHECK(usb_host_endpoint_flush(usb_midi->device, usb_midi->in.endpoint->bEndpointAddress));
    usb_midi->in.endpoint = NULL;

    ESP_ERROR_CHECK(usb_host_endpoint_halt(usb_midi->device, usb_midi->out.endpoint->bEndpointAddress));
    ESP_ERROR_CHECK(usb_host_endpoint_flush(usb_midi->device, usb_midi->out.endpoint->bEndpointAddress));
    usb_midi->out.endpoint = NULL;

    // release the interface
    ESP_ERROR_CHECK(usb_host_interface_release(usb_midi->client, usb_midi->device, usb_midi->interface->bInterfaceNumber));
    usb_midi->interface = NULL;

    // close the device
    ESP_LOGI(TAG, "closing device 0x%02x", usb_midi->device_address);
    ESP_ERROR_CHECK(usb_host_device_close(usb_midi->client, usb_midi->device));

    // free the data buffers
    vQueueDelete(usb_midi->in.packet_queue);
    free(usb_midi->in.sysex_buffer);

    vQueueDelete(usb_midi->out.packet_queue);
    free(usb_midi->out.sysex_buffer);

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
        // handle usb events
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
    usb_midi->state = USB_MIDI_DISCONNECTED;

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
    usb_midi->out.transfer->num_bytes = 4;
    usb_midi->out.transfer->timeout_ms = 10000;
    memset(usb_midi->out.transfer->data_buffer, 0, 4);

    // set the cin
    usb_midi->out.transfer->data_buffer[0] = message->command >> 4;

    // encode the message
    err = midi_message_encode(message, usb_midi->out.transfer->data_buffer + 1, 3);
    if (err != ESP_OK) return err;

    // submit the transfer
    return usb_host_transfer_submit(usb_midi->out.transfer);
}
