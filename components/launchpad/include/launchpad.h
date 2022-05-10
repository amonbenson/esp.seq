#pragma once

#include "usb_midi.h"
#include <usb/usb_host.h>
#include <esp_err.h>


#define LAUNCHPAD_VENDOR_ID 0x1235
#define LAUNCHPAD_PRO_PRODUCT_ID 0x51


typedef struct {
    usb_midi_t *usb_midi;
} launchpad_config_t;

typedef struct {
    launchpad_config_t config;
    bool connected;
} launchpad_t;


esp_err_t launchpad_pro_clear(launchpad_t *launchpad);

esp_err_t launchpad_connected_callback(launchpad_t *launchpad, const usb_device_desc_t *device_descriptor);

esp_err_t launchpad_disconnected_callback(launchpad_t *launchpad);

esp_err_t launchpad_recv_callback(launchpad_t *launchpad, const midi_message_t *message);

esp_err_t launchpad_init(const launchpad_config_t *config, launchpad_t *launchpad);
