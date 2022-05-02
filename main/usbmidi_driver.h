#pragma once

#include <usb/usb_host.h>
#include <freertos/queue.h>


#define USBMIDI_EVENT_QUEUE_LEN 5


typedef enum {
    USBMIDI_EVENT_OPEN_DEV,
    USBMIDI_EVENT_CLOSE_DEV,
    USBMIDI_EVENT_GET_DEV_INFO,
    USBMIDI_EVENT_GET_DEV_DESC
} usbmidi_event_t;

typedef struct {
    usb_host_client_handle_t client;
    usb_device_handle_t device;
    uint8_t device_address;
    QueueHandle_t events;
} usbmidi_driver_t;


void usbmidi_driver_task(void *arg);
