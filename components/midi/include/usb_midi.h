#pragma once

#include <usb/usb_host.h>
#include "usb.h"


#define USB_SUBCLASS_MIDISTREAMING 0x03

#define USB_MIDI_PACKET_SIZE 64

#define USB_MIDI_CIN_MISC 0x0
#define USB_MIDI_CIN_CABLE_EVENT 0x1
#define USB_MIDI_CIN_SYSCOM_2 0x2
#define USB_MIDI_CIN_SYSCOM_2_LEN 2
#define USB_MIDI_CIN_SYSCOM_3 0x3
#define USB_MIDI_CIN_SYSCOM_3_LEN 3
#define USB_MIDI_CIN_SYSEX_START 0x4
#define USB_MIDI_CIN_SYSEX_START_LEN 3
#define USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1 0x5
#define USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1_LEN 1
#define USB_MIDI_CIN_SYSEX_END_2 0x6
#define USB_MIDI_CIN_SYSEX_END_2_LEN 2
#define USB_MIDI_CIN_SYSEX_END_3 0x7
#define USB_MIDI_CIN_SYSEX_END_3_LEN 3
#define USB_MIDI_CIN_NOTE_OFF 0x8
#define USB_MIDI_CIN_NOTE_OFF_LEN 3
#define USB_MIDI_CIN_NOTE_ON 0x9
#define USB_MIDI_CIN_NOTE_ON_LEN 3
#define USB_MIDI_CIN_POLY_KEY_PRESSURE 0xA
#define USB_MIDI_CIN_POLY_KEY_PRESSURE_LEN 3
#define USB_MIDI_CIN_CONTROL_CHANGE 0xB
#define USB_MIDI_CIN_CONTROL_CHANGE_LEN 3
#define USB_MIDI_CIN_PROGRAM_CHANGE 0xC
#define USB_MIDI_CIN_PROGRAM_CHANGE_LEN 2
#define USB_MIDI_CIN_CHANNEL_PRESSURE 0xD
#define USB_MIDI_CIN_CHANNEL_PRESSURE_LEN 2
#define USB_MIDI_CIN_PITCH_BEND 0xE
#define USB_MIDI_CIN_PITCH_BEND_LEN 3
#define USB_MIDI_CIN_BYTE 0xF
#define USB_MIDI_CIN_BYTE_LEN 1


typedef void (*usb_midi_device_connected_callback_t)(usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_device_disconnected_callback_t)(usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_note_on_callback_t)(uint8_t channel, uint8_t note, uint8_t velocity);
typedef void (*usb_midi_note_off_callback_t)(uint8_t channel, uint8_t note, uint8_t velocity);
typedef void (*usb_midi_poly_key_pressure_callback_t)(uint8_t channel, uint8_t note, uint8_t pressure);
typedef void (*usb_midi_control_change_callback_t)(uint8_t channel, uint8_t control, uint8_t value);
typedef void (*usb_midi_program_change_callback_t)(uint8_t channel, uint8_t program);
typedef void (*usb_midi_channel_pressure_callback_t)(uint8_t channel, uint8_t pressure);
typedef void (*usb_midi_pitch_bend_callback_t)(uint8_t channel, int16_t value);

typedef struct {
    usb_midi_device_connected_callback_t connected;
    usb_midi_device_disconnected_callback_t disconnected;
    usb_midi_note_on_callback_t note_on;
    usb_midi_note_off_callback_t note_off;
    usb_midi_poly_key_pressure_callback_t poly_key_pressure;
    usb_midi_control_change_callback_t control_change;
    usb_midi_program_change_callback_t program_change;
    usb_midi_channel_pressure_callback_t channel_pressure;
    usb_midi_pitch_bend_callback_t pitch_bend;
} usb_midi_callbacks_t;

typedef struct {
    usb_midi_callbacks_t callbacks;
} usb_midi_config_t;

typedef struct {
    usb_driver_config_t driver_config;
    usb_midi_config_t config;

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
