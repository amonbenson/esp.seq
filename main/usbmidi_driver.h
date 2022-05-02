#pragma once

#include <usb/usb_host.h>
#include <freertos/queue.h>


#define USB_SUBCLASS_MIDISTREAMING 0x03
#define USBMIDI_EVENT_QUEUE_LEN 5
#define USBMIDI_PACKET_SIZE 64


typedef enum {
    USBMIDI_EVENT_OPEN_DEV,
    USBMIDI_EVENT_CLOSE_DEV,
    USBMIDI_EVENT_GET_DEV_INFO,
    USBMIDI_EVENT_GET_DEV_DESC,
    USBMIDI_EVENT_GET_CONFIG_DESC,
    USBMIDI_EVENT_GET_STR_DESC
} usbmidi_event_t;

typedef struct {
    QueueHandle_t events;

    usb_host_client_handle_t client;
    usb_device_handle_t device;
    uint8_t device_address;
    const usb_device_desc_t *device_descriptor;

    const usb_intf_desc_t *interface;
    usb_transfer_t *data_in;
    usb_transfer_t *data_out;
} usbmidi_driver_t;


void usbmidi_driver_task(void *arg);
