#include "usb_midi.h"
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_check.h>
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

void usb_midi_in_task(void *arg) {
    usb_midi_t *usb_midi = (usb_midi_t *) arg;
    /* usb_midi_packet_t packet;
    midi_message_t message; */

    while (1) {
        vTaskDelay(portMAX_DELAY);
    }

    vTaskDelete(NULL);
}

void usb_midi_out_task(void *arg) {
    usb_midi_t *usb_midi = (usb_midi_t *) arg;
    usb_midi_packet_t packet;
    midi_message_t message;

    while (1) {
        // TODO: use some kind of notification!!!
        //while (usb_midi->state != USB_MIDI_CONNECTED) vTaskDelay(100);

        // wait for the next packet
        xQueueReceive(usb_midi->out.packet_queue, &packet, portMAX_DELAY);

        // handle the packet
        xSemaphoreTake(usb_midi->lock, portMAX_DELAY);
        ESP_LOGI(TAG, "PACKET AVAILABLE");

        midi_message_decode(packet.data, 3, &message);
        midi_message_print(&message);

        xSemaphoreGive(usb_midi->lock);
    }
}

static esp_err_t usb_midi_port_init(usb_midi_t *usb_midi, usb_midi_port_t *port, const usb_ep_desc_t *endpoint, usb_transfer_cb_t transfer_callback) {
    usb_transfer_t *transfer;

    // allocate the transfer
    ESP_RETURN_ON_ERROR(usb_host_transfer_alloc(USB_MIDI_TRANSFER_MAX_SIZE, 0, &transfer),
        TAG, "failed to allocate transfer");
    transfer->device_handle = usb_midi->device;
    transfer->bEndpointAddress = endpoint->bEndpointAddress;
    transfer->callback = transfer_callback;
    transfer->context = usb_midi;
    transfer->num_bytes = USB_MIDI_TRANSFER_MAX_SIZE;

    port->endpoint = endpoint;
    port->transfer = transfer;

    // allocate the data buffers
    port->packet_queue = xQueueCreate(USB_MIDI_PACKET_QUEUE_SIZE, sizeof(usb_midi_packet_t));
    ESP_RETURN_ON_FALSE(port->packet_queue, ESP_ERR_NO_MEM,
        TAG, "failed to create packet queue");

    port->sysex_buffer = malloc(USB_MIDI_SYSEX_BUFFER_SIZE);
    ESP_RETURN_ON_FALSE(port->sysex_buffer, ESP_ERR_NO_MEM,
        TAG, "failed to allocate sysex buffer");
    port->sysex_len = 0;

    return ESP_OK;
}

static esp_err_t usb_midi_port_destroy(usb_midi_t *usb_midi, usb_midi_port_t *port) {
    // free the transfer and endpoint
    ESP_RETURN_ON_ERROR(usb_host_transfer_free(port->transfer),
        TAG, "failed to free transfer");

    ESP_RETURN_ON_ERROR(usb_host_endpoint_halt(usb_midi->device, port->endpoint->bEndpointAddress),
        TAG, "failed to halt endpoint");
    ESP_RETURN_ON_ERROR(usb_host_endpoint_flush(usb_midi->device, port->endpoint->bEndpointAddress),
        TAG, "failed to flush endpoint");

    // free the data buffers
    vQueueDelete(port->packet_queue);
    free(port->sysex_buffer);

    return ESP_OK;
}

static esp_err_t usb_midi_open_device(usb_midi_t *usb_midi, uint8_t address) {
    esp_err_t ret;
    usb_device_info_t info;
    const usb_config_desc_t *descriptor;
    const usb_standard_desc_t *d;
    int offset = 0;

    const usb_intf_desc_t *interface = NULL;
    const usb_ep_desc_t *data_in = NULL;
    const usb_ep_desc_t *data_out = NULL;

    xSemaphoreTake(usb_midi->lock, portMAX_DELAY);

    // make sure the device is not connected already
    ESP_GOTO_ON_FALSE(usb_midi->state == USB_MIDI_DISCONNECTED, ESP_ERR_INVALID_STATE, exit,
        TAG, "device is already connected");

    // open the device
    usb_midi->device_address = address;
    ESP_LOGI(TAG, "opening device at 0x%02x", usb_midi->device_address);
    ESP_GOTO_ON_ERROR(usb_host_device_open(usb_midi->client, usb_midi->device_address, &usb_midi->device), exit,
        TAG, "failed to open device");

    // get the device info
    ESP_LOGI(TAG, "getting device information");
    ESP_GOTO_ON_ERROR(usb_host_device_info(usb_midi->device, &info), exit,
        TAG, "failed to get device information");

    // get the device descriptor
    ESP_LOGI(TAG, "getting device descriptor");
    ESP_GOTO_ON_ERROR(usb_host_get_device_descriptor(usb_midi->device, &usb_midi->device_descriptor), exit,
        TAG, "failed to get device descriptor");

    // get the configuration descriptor
    assert(usb_midi->device != NULL);
    ESP_LOGI(TAG, "getting configuration descriptor");
    ESP_GOTO_ON_ERROR(usb_host_get_active_config_descriptor(usb_midi->device, &descriptor), exit,
        TAG, "failed to get configuration descriptor");

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
    ESP_GOTO_ON_FALSE(interface && data_in && data_out, ESP_ERR_NOT_SUPPORTED, exit,
        TAG, "not a valid midi device");
    
    // claim the interface
    ESP_LOGI(TAG, "claiming interface");
    ESP_GOTO_ON_ERROR(usb_host_interface_claim(usb_midi->client,
            usb_midi->device,
            interface->bInterfaceNumber,
            interface->bAlternateSetting), exit,
        TAG, "failed to claim interface");

    // allocate the ports
    ESP_LOGI(TAG, "creating ports");
    ESP_GOTO_ON_ERROR(usb_midi_port_init(usb_midi, &usb_midi->in, data_in, usb_midi_data_in_callback), exit,
        TAG, "failed to create data in port");
    ESP_GOTO_ON_ERROR(usb_midi_port_init(usb_midi, &usb_midi->out, data_out, usb_midi_data_out_callback), exit,
        TAG, "failed to create data out port");

    // mark as connected and invoke the callback
    usb_midi->state = USB_MIDI_CONNECTED;
    MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, connected, usb_midi->device_descriptor);

    // init the io queue handler tasks
    xTaskCreatePinnedToCore(usb_midi_in_task, "usb_midi_in", 2048, (void *) usb_midi, 3, usb_midi->in.task, 0);
    xTaskCreatePinnedToCore(usb_midi_out_task, "usb_midi_out", 2048, (void *) usb_midi, 3, usb_midi->out.task, 0);

    // start input polling
    ESP_GOTO_ON_ERROR(usb_host_transfer_submit(usb_midi->in.transfer), exit,
        TAG, "failed to submit data in transfer");

    ESP_LOGI(TAG, "midi device initialized");
    ret = ESP_OK;

exit:
    xSemaphoreGive(usb_midi->lock);
    return ret;
}

static esp_err_t usb_midi_close_device(usb_midi_t *usb_midi) {
    esp_err_t ret;

    xSemaphoreTake(usb_midi->lock, portMAX_DELAY);

    ESP_GOTO_ON_FALSE(usb_midi->state == USB_MIDI_CONNECTED, ESP_ERR_INVALID_STATE, exit,
        TAG, "device is not connected");

    // kill the io queue handler tasks
    vTaskDelete(usb_midi->in.task);
    vTaskDelete(usb_midi->out.task);

    // mark as disconnected and invoke the callback
    usb_midi->state = USB_MIDI_DISCONNECTED;
    MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, disconnected, usb_midi->device_descriptor);

    // free the ports
    ESP_GOTO_ON_ERROR(usb_midi_port_destroy(usb_midi, &usb_midi->in), exit,
        TAG, "failed to free input port");
    ESP_GOTO_ON_ERROR(usb_midi_port_destroy(usb_midi, &usb_midi->out), exit,
        TAG, "failed to free output port");

    // release the interface
    ESP_GOTO_ON_ERROR(usb_host_interface_release(usb_midi->client, usb_midi->device, usb_midi->interface->bInterfaceNumber), exit,
        TAG, "failed to release interface");

    // close the device
    ESP_LOGI(TAG, "closing device 0x%02x", usb_midi->device_address);
    ESP_GOTO_ON_ERROR(usb_host_device_close(usb_midi->client, usb_midi->device), exit,
        TAG, "failed to close device");

    ret = ESP_OK;
exit:
    xSemaphoreGive(usb_midi->lock);
    return ret;
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

    // store the config and initial state
    usb_midi->config = *config;
    usb_midi->state = USB_MIDI_DISCONNECTED;
    usb_midi->lock = xSemaphoreCreateMutex();

    // link the driver task function and argument, this will be dispatched from the usb interface
    usb_midi->driver_config.task = usb_midi_driver_task;
    usb_midi->driver_config.arg = (void *) usb_midi;

    return ESP_OK;
}

esp_err_t usb_midi_send(usb_midi_t *usb_midi, const midi_message_t *message) {
    esp_err_t err;
    usb_midi_packet_t packet;

    if (!MIDI_COMMAND_IS_CHANNEL_VOICE(message->command)) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    xSemaphoreTake(usb_midi->lock, portMAX_DELAY);

    // make sure we are connected
    if (usb_midi->state != USB_MIDI_CONNECTED) {
        err = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    // set the cin
    packet.cn_cin = message->command >> 4;

    // encode the message
    err = midi_message_encode(message, packet.data, 3);
    if (err != ESP_OK) goto exit;

    // queue the packet
    xQueueSend(usb_midi->out.packet_queue, &packet, portMAX_DELAY);

    /*
    // init the output buffer
    usb_midi->out.transfer->num_bytes = 4;
    usb_midi->out.transfer->timeout_ms = 10000;

    // submit the transfer
    err = usb_host_transfer_submit(usb_midi->out.transfer);
    if (err != ESP_OK) goto exit;
    */

    err = ESP_OK;
exit:
    xSemaphoreGive(usb_midi->lock);
    return err;
}
