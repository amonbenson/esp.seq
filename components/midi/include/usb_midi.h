#pragma once

#include <esp_err.h>
#include <esp_log.h>
#include <usb/usb_host.h>
#include "midi_message.h"
#include "usb.h"


#define USB_SUBCLASS_MIDISTREAMING 0x03

#define USB_MIDI_PACKET_MAX_LEN 64
#define USB_MIDI_SYSEX_MAX_LEN 256

typedef enum {
    USB_MIDI_CIN_MISC = 0x0,
    USB_MIDI_CIN_CABLE_EVENT = 0x1,
    USB_MIDI_CIN_SYSCOM_2 = 0x2,
    USB_MIDI_CIN_SYSCOM_3 = 0x3,
    USB_MIDI_CIN_SYSEX_START = 0x4,
    USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1 = 0x5,
    USB_MIDI_CIN_SYSEX_END_2 = 0x6,
    USB_MIDI_CIN_SYSEX_END_3 = 0x7,
    USB_MIDI_CIN_NOTE_OFF = 0x8,
    USB_MIDI_CIN_NOTE_ON = 0x9,
    USB_MIDI_CIN_POLY_KEY_PRESSURE = 0xA,
    USB_MIDI_CIN_CONTROL_CHANGE = 0xB,
    USB_MIDI_CIN_PROGRAM_CHANGE = 0xC,
    USB_MIDI_CIN_CHANNEL_PRESSURE = 0xD,
    USB_MIDI_CIN_PITCH_BEND = 0xE,
    USB_MIDI_CIN_BYTE = 0xF
} udb_midi_cin_t;


/* typedef void (*usb_midi_device_connected_callback_t)(const usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_device_disconnected_callback_t)(const usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_sysex_callback_t)(const uint8_t *data, size_t length);
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
    usb_midi_sysex_callback_t sysex;
    usb_midi_note_on_callback_t note_on;
    usb_midi_note_off_callback_t note_off;
    usb_midi_poly_key_pressure_callback_t poly_key_pressure;
    usb_midi_control_change_callback_t control_change;
    usb_midi_program_change_callback_t program_change;
    usb_midi_channel_pressure_callback_t channel_pressure;
    usb_midi_pitch_bend_callback_t pitch_bend;
} usb_midi_callbacks_t; */

typedef void (*usb_midi_device_connected_callback_t)(const usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_device_disconnected_callback_t)(const usb_device_desc_t *device_descriptor);
typedef void (*usb_midi_recv_callback_t)(const midi_message_t *message);

typedef struct {
    struct {
        usb_midi_device_connected_callback_t connected;
        usb_midi_device_disconnected_callback_t disconnected;
        usb_midi_recv_callback_t recv;
    } callbacks;
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

    uint8_t sysex_buffer[USB_MIDI_SYSEX_MAX_LEN];
    size_t sysex_len;
} usb_midi_t;


void usb_midi_driver_task(void *arg);

esp_err_t usb_midi_init(const usb_midi_config_t *config, usb_midi_t *usb_midi);

esp_err_t usb_midi_send(usb_midi_t *usb_midi, const midi_message_t *message);
