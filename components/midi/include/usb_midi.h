#pragma once

#include <usb/usb_host.h>
#include <freertos/queue.h>
#include "usb.h"


#define USB_SUBCLASS_MIDISTREAMING 0x03
#define USBMIDI_EVENT_QUEUE_LEN 5

#define USBMIDI_PACKET_SIZE 64

#define USBMIDI_CIN_MISC 0x0
#define USBMIDI_CIN_CABLE_EVENT 0x1
#define USBMIDI_CIN_SYSCOM_2 0x2
#define USBMIDI_CIN_SYSCOM_2_LEN 2
#define USBMIDI_CIN_SYSCOM_3 0x3
#define USBMIDI_CIN_SYSCOM_3_LEN 3
#define USBMIDI_CIN_SYSEX_START 0x4
#define USBMIDI_CIN_SYSEX_START_LEN 3
#define USBMIDI_CIN_SYSEX_END_1_SYSCOM_1 0x5
#define USBMIDI_CIN_SYSEX_END_1_SYSCOM_1_LEN 1
#define USBMIDI_CIN_SYSEX_END_2 0x6
#define USBMIDI_CIN_SYSEX_END_2_LEN 2
#define USBMIDI_CIN_SYSEX_END_3 0x7
#define USBMIDI_CIN_SYSEX_END_3_LEN 3
#define USBMIDI_CIN_NOTE_OFF 0x8
#define USBMIDI_CIN_NOTE_OFF_LEN 3
#define USBMIDI_CIN_NOTE_ON 0x9
#define USBMIDI_CIN_NOTE_ON_LEN 3
#define USBMIDI_CIN_POLY_KEY_PRESSURE 0xA
#define USBMIDI_CIN_POLY_KEY_PRESSURE_LEN 3
#define USBMIDI_CIN_CONTROL_CHANGE 0xB
#define USBMIDI_CIN_CONTROL_CHANGE_LEN 3
#define USBMIDI_CIN_PROGRAM_CHANGE 0xC
#define USBMIDI_CIN_PROGRAM_CHANGE_LEN 2
#define USBMIDI_CIN_CHANNEL_PRESSURE 0xD
#define USBMIDI_CIN_CHANNEL_PRESSURE_LEN 2
#define USBMIDI_CIN_PITCH_BEND 0xE
#define USBMIDI_CIN_PITCH_BEND_LEN 3
#define USBMIDI_CIN_BYTE 0xF
#define USBMIDI_CIN_BYTE_LEN 1


typedef enum {
    USBMIDI_EVENT_OPEN_DEV,
    USBMIDI_EVENT_CLOSE_DEV,
    USBMIDI_EVENT_GET_DEV_INFO,
    USBMIDI_EVENT_GET_DEV_DESC,
    USBMIDI_EVENT_GET_CONFIG_DESC,
    USBMIDI_EVENT_GET_STR_DESC
} usb_midi_event_t;

typedef void (*usb_midi_device_connected_callback_t)(usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_device_disconnected_callback_t)(usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_note_on_callback_t)(uint8_t channel, uint8_t note, uint8_t velocity);
typedef void (*usb_midi_note_off_callback_t)(uint8_t channel, uint8_t note, uint8_t velocity);

typedef struct {
    usb_midi_device_connected_callback_t connected;
    usb_midi_device_disconnected_callback_t disconnected;
    usb_midi_note_on_callback_t note_on;
    usb_midi_note_off_callback_t note_off;
} usb_midi_callbacks_t;

typedef struct {
    usb_midi_callbacks_t callbacks;
} usb_midi_config_t;

typedef struct {
    usb_driver_config_t driver_config;
    usb_midi_config_t config;
    QueueHandle_t events;

    usb_host_client_handle_t client;
    usb_device_handle_t device;
    uint8_t device_address;
    const usb_device_desc_t *device_descriptor;

    const usb_intf_desc_t *interface;
    usb_transfer_t *data_in;
    usb_transfer_t *data_out;
} usb_midi_t;


void usb_midi_driver_task(void *arg);

esp_err_t usb_midi_init(const usb_midi_config_t *config, usb_midi_t *usb_midi);
