#pragma once

#include <usb/usb_host.h>
#include <freertos/queue.h>


#define USBMIDI_ACTION_OPEN_DEV 0x01
#define USBMIDI_ACTION_CLOSE_DEV 0x20


typedef struct {
    usb_host_client_handle_t client;
    usb_device_handle_t device;
    uint8_t device_address;
    uint32_t actions;
} usbmidi_driver_t;


void usbmidi_driver_task(void *arg);
