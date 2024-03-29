#include "usb_midi.h"
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>
#include <usb/usb_host.h>
#include <string.h>
#include "midi.h"
#include "midi_types.h"
#include "midi_message.h"


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


static const char *TAG = "usb_midi";
static const char *TAG_IN = "usb_midi: data in";
static const char *TAG_OUT = "usb_midi: data out";


static void usb_midi_sysex_reset(usb_midi_t *usb_midi) {
    // clear the buffer by resetting the length
    usb_midi->in.sysex_len = 0;
}

static esp_err_t usb_midi_sysex_parse(usb_midi_t *usb_midi, const uint8_t *data, size_t len) {
    midi_message_t message;
    usb_midi_port_t *in = &usb_midi->in;

    for (size_t i = 0; i < len && in->sysex_len < USB_MIDI_SYSEX_BUFFER_SIZE; i++, in->sysex_len++) {
        // store the incoming byte
        in->sysex_buffer[in->sysex_len] = data[i];

        // if we've reached the stop byte, invoke the callback
        if (data[i] == 0xf7) {
            in->sysex_len++;

            ESP_RETURN_ON_ERROR(midi_message_decode(in->sysex_buffer, in->sysex_len, &message),
                TAG, "failed to decode sysex message");

            MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, recv, &message);
            usb_midi_sysex_reset(usb_midi);
        }
    }

    return ESP_OK;
}

static esp_err_t usb_midi_parse_packet(usb_midi_t *usb_midi, const usb_midi_packet_t *packet) {
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
        ESP_RETURN_ON_ERROR(midi_message_decode(data, 3, &message),
            TAG, "failed to decode short message");

        // cin number should correspond to the midi command
        ESP_RETURN_ON_FALSE(cin == message.command >> 4, ESP_ERR_INVALID_ARG,
            TAG, "invalid cin number for short message");

        MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, recv, &message);
        return ESP_OK;
    }

    // handle special messages
    switch (cin) {
        case USB_MIDI_CIN_SYSEX_START_CONT:
            ESP_RETURN_ON_ERROR(usb_midi_sysex_parse(usb_midi, data, 3),
                TAG, "failed to parse sysex start packet");
            break;
        case USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1:
            ESP_RETURN_ON_ERROR(usb_midi_sysex_parse(usb_midi, data, 1),
                TAG, "failed to parse sysex end packet");
            usb_midi_sysex_reset(usb_midi);
            break;
        case USB_MIDI_CIN_SYSEX_END_2:
            ESP_RETURN_ON_ERROR(usb_midi_sysex_parse(usb_midi, data, 2),
                TAG, "failed to parse sysex end packet");
            usb_midi_sysex_reset(usb_midi);
            break;
        case USB_MIDI_CIN_SYSEX_END_3:
            ESP_RETURN_ON_ERROR(usb_midi_sysex_parse(usb_midi, data, 3),
                 TAG, "failed to parse sysex end packet");
            usb_midi_sysex_reset(usb_midi);
            break;
        default:
            ESP_RETURN_ON_ERROR(ESP_ERR_INVALID_ARG, TAG, "unsupported cin code (%d)", cin);
    }

    return ESP_OK;
}

static void usb_midi_data_in_callback(usb_transfer_t *transfer) {
    esp_err_t ret;
    usb_midi_t *usb_midi = (usb_midi_t *) transfer->context;
    const usb_midi_packet_t *packet;
    int i, n;

    //xSemaphoreTake(usb_midi->lock, portMAX_DELAY);

    // device is not valid anymore
    ESP_GOTO_ON_FALSE(usb_midi->state == USB_MIDI_CONNECTED, ESP_ERR_INVALID_STATE, exit,
        TAG_IN, "device is not connected anymore");

    // validate the size
    n = transfer->actual_num_bytes / 4;
    ESP_GOTO_ON_FALSE(n > 0, ESP_ERR_INVALID_SIZE, exit,
        TAG_IN, "invalid packet size (%d)", transfer->actual_num_bytes);

    // handle the incoming data in 4 byte packets
    for (i = 0, packet = (usb_midi_packet_t *) transfer->data_buffer; i < n; i++, packet++) {
        ESP_GOTO_ON_ERROR(usb_midi_parse_packet(usb_midi, packet), exit,
            TAG_IN, "failed to parse packet");
    }

    // continue polling
    ESP_GOTO_ON_ERROR(usb_host_transfer_submit(usb_midi->in.transfer), exit,
        TAG_IN, "failed to submit transfer");

    ret = ESP_OK;
exit:
    //xSemaphoreGive(usb_midi->lock);
    return;
}

static void usb_midi_data_out_callback(usb_transfer_t *transfer) {
    usb_midi_t *usb_midi = (usb_midi_t *) transfer->context;

    // release the transfer lock
    xSemaphoreGive(usb_midi->transfer_lock);
    //ESP_LOGI(TAG_OUT, "data out callback");
}

void usb_midi_out_task(void *arg) {
    usb_midi_t *usb_midi = (usb_midi_t *) arg;
    usb_midi_packet_t packet;
    size_t i, transfer_size;

    while (1) {
        // wait for new data
        xQueuePeek(usb_midi->out.packet_queue, &packet, portMAX_DELAY);
        xSemaphoreTake(usb_midi->lock, portMAX_DELAY);
        xSemaphoreTake(usb_midi->transfer_lock, portMAX_DELAY);

        // prepare the transfer
        transfer_size = MIN(uxQueueMessagesWaiting(usb_midi->out.packet_queue) * sizeof(usb_midi_packet_t), USB_MIDI_TRANSFER_MAX_SIZE);
        usb_midi->out.transfer->num_bytes = transfer_size;
        for (i = 0; i < transfer_size; i += sizeof(usb_midi_packet_t)) {
            assert(xQueueReceive(usb_midi->out.packet_queue, &usb_midi->out.transfer->data_buffer[i], 0) == pdTRUE);
        }

        // submit the transfer
        //ESP_LOGI(TAG_OUT, "transfering %d bytes", transfer_size);
        usb_host_transfer_submit(usb_midi->out.transfer);

        // NOTE: transfer_lock will be released from usb_midi_data_out_callback
        xSemaphoreGive(usb_midi->lock);
        taskYIELD();
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
    usb_midi->interface = interface;

    // allocate the ports
    ESP_LOGI(TAG, "creating ports");
    ESP_GOTO_ON_ERROR(usb_midi_port_init(usb_midi, &usb_midi->in, data_in, usb_midi_data_in_callback), exit,
        TAG, "failed to create data in port");
    ESP_GOTO_ON_ERROR(usb_midi_port_init(usb_midi, &usb_midi->out, data_out, usb_midi_data_out_callback), exit,
        TAG, "failed to create data out port");

    // mark as connected and invoke the callback
    usb_midi->state = USB_MIDI_CONNECTED;
    xSemaphoreGive(usb_midi->lock);
    MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, connected, usb_midi->device_descriptor);
    xSemaphoreTake(usb_midi->lock, portMAX_DELAY);

    // start input polling
    ESP_GOTO_ON_ERROR(usb_host_transfer_submit(usb_midi->in.transfer), exit,
        TAG, "failed to submit data in transfer");

    // start the transfer handler task
    xSemaphoreGive(usb_midi->transfer_lock);
    if (usb_midi->transfer_task == NULL) {
        xTaskCreatePinnedToCore(usb_midi_out_task, "usb_midi_transfer", 2048, (void *) usb_midi, 1, usb_midi->transfer_task, 0);
    }

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

    // mark as disconnected and invoke the callback
    usb_midi->state = USB_MIDI_DISCONNECTED;
    xSemaphoreGive(usb_midi->lock);
    MIDI_INVOKE_CALLBACK(&usb_midi->config.callbacks, disconnected, usb_midi->device_descriptor);
    xSemaphoreTake(usb_midi->lock, portMAX_DELAY);

    // free the ports
    ESP_GOTO_ON_ERROR(usb_midi_port_destroy(usb_midi, &usb_midi->in), exit,
        TAG, "failed to free input port");
    ESP_GOTO_ON_ERROR(usb_midi_port_destroy(usb_midi, &usb_midi->out), exit,
        TAG, "failed to free output port");

    // release the interface (seems to be NULL sometimes?)
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
    esp_err_t ret;
    usb_midi_t *usb_midi = (usb_midi_t *) arg;

    switch (msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            ESP_GOTO_ON_ERROR(usb_midi_open_device(usb_midi, msg->new_dev.address), exit,
                TAG, "failed to open device");
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            ESP_GOTO_ON_ERROR(usb_midi_close_device(usb_midi), exit,
                TAG, "failed to close device");
            break;
        default:
            ESP_GOTO_ON_ERROR(ESP_ERR_INVALID_ARG, exit,
                TAG, "unknown client event %d", msg->event);
            break;
    }

    ret = ESP_OK;
exit:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "client event %d failed with error %s", msg->event, esp_err_to_name(ret));
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
    usb_midi->transfer_lock = xSemaphoreCreateBinary();

    // link the driver task function and argument, this will be dispatched from the usb interface
    usb_midi->driver_config.task = usb_midi_driver_task;
    usb_midi->driver_config.arg = (void *) usb_midi;

    return ESP_OK;
}

static esp_err_t usb_midi_queue_out_packet(usb_midi_t *usb_midi, usb_midi_packet_t *packet) {
    if (xQueueSend(usb_midi->out.packet_queue, packet, 0)) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t usb_midi_send(usb_midi_t *usb_midi, const midi_message_t *message) {
    esp_err_t ret;
    usb_midi_packet_t packet = { 0 };
    const uint8_t *sysex_data;
    size_t sysex_length;

    ESP_GOTO_ON_FALSE(MIDI_COMMAND_IS_VALID(message->command), ESP_ERR_INVALID_ARG, exit,
        TAG_OUT, "invalid midi command");

    xSemaphoreTake(usb_midi->lock, portMAX_DELAY);

    // make sure we are connected
    ESP_GOTO_ON_FALSE(usb_midi->state == USB_MIDI_CONNECTED, ESP_ERR_INVALID_STATE, exit,
        TAG_OUT, "device is not connected");

    // handle short messages
    if (MIDI_COMMAND_IS_CHANNEL_VOICE(message->command)) {
        packet.cn_cin = message->command >> 4;
        ESP_GOTO_ON_ERROR(midi_message_encode(message, packet.data, 3), exit,
            TAG_OUT, "failed to encode message");
        ESP_GOTO_ON_ERROR(usb_midi_queue_out_packet(usb_midi, &packet), exit,
            TAG_OUT, "queue full");

        ret = ESP_OK;
        goto exit;
    }

    // handle special messages
    switch (message->command) {
        case MIDI_COMMAND_SYSEX:
            sysex_data = message->sysex.data;
            sysex_length = message->sysex.length;

            ESP_GOTO_ON_FALSE(sysex_length > 0, ESP_ERR_INVALID_SIZE, exit,
                TAG_OUT, "sysex message has no data");

            // split the sysex message into packets of 3 bytes each
            while (sysex_length > 3) {
                packet.cn_cin = USB_MIDI_CIN_SYSEX_START_CONT;
                memcpy(packet.data, sysex_data, 3);
                ESP_GOTO_ON_ERROR(usb_midi_queue_out_packet(usb_midi, &packet), exit,
                    TAG_OUT, "queue full");

                sysex_data += 3;
                sysex_length -= 3;
            }

            // store the trailing packet
            packet.cn_cin = USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1 + sysex_length - 1;
            memcpy(packet.data, sysex_data, sysex_length);
            ESP_GOTO_ON_ERROR(usb_midi_queue_out_packet(usb_midi, &packet), exit,
                TAG_OUT, "queue full");

            break;
        default:
            ESP_GOTO_ON_ERROR(ESP_ERR_NOT_SUPPORTED, exit,
                TAG_OUT, "unsupported midi command %d", message->command);
            break;
    }

    ret = ESP_OK;
exit:
    xSemaphoreGive(usb_midi->lock);
    return ret;
}

esp_err_t usb_midi_send_sysex(usb_midi_t *usb_midi, const uint8_t *data, size_t length) {
    midi_message_t message = {
        .command = MIDI_COMMAND_SYSEX,
        .sysex = {
            .data = data,
            .length = length
        }
    };
    return usb_midi_send(usb_midi, &message);
}
