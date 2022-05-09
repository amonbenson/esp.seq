#pragma once

#include <esp_err.h>
#include <esp_log.h>
#include <usb/usb_host.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "midi_message.h"
#include "usb.h"


#define USB_SUBCLASS_MIDISTREAMING 0x03

#define USB_MIDI_TRANSFER_MAX_SIZE 64
#define USB_MIDI_SYSEX_BUFFER_SIZE 256
#define USB_MIDI_PACKET_QUEUE_SIZE 64

#define USB_MIDI_CIN_MISC 0x0
#define USB_MIDI_CIN_CABLE_EVENT 0x1
#define USB_MIDI_CIN_SYSCOM_2 0x2
#define USB_MIDI_CIN_SYSCOM_3 0x3
#define USB_MIDI_CIN_SYSEX_START_CONT 0x4
#define USB_MIDI_CIN_SYSEX_END_1_SYSCOM_1 0x5
#define USB_MIDI_CIN_SYSEX_END_2 0x6
#define USB_MIDI_CIN_SYSEX_END_3 0x7
#define USB_MIDI_CIN_NOTE_OFF 0x8
#define USB_MIDI_CIN_NOTE_ON 0x9
#define USB_MIDI_CIN_POLY_KEY_PRESSURE 0xA
#define USB_MIDI_CIN_CONTROL_CHANGE 0xB
#define USB_MIDI_CIN_PROGRAM_CHANGE 0xC
#define USB_MIDI_CIN_CHANNEL_PRESSURE 0xD
#define USB_MIDI_CIN_PITCH_BEND 0xE
#define USB_MIDI_CIN_BYTE 0xF

#define USB_MIDI_LOCK(usb_midi) xSemaphoreTake((usb_midi)->lock, portMAX_DELAY)
#define USB_MIDI_UNLOCK(usb_midi) xSemaphoreGive((usb_midi)->lock)


typedef uint8_t usb_midi_cin_t;

typedef struct {
    uint8_t cn_cin;
    uint8_t data[3];
} __attribute__((packed)) usb_midi_packet_t;


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
    const usb_ep_desc_t *endpoint;
    usb_transfer_t *transfer;

    QueueHandle_t packet_queue;

    uint8_t *sysex_buffer;
    size_t sysex_len;
} usb_midi_port_t;

typedef enum {
    USB_MIDI_DISCONNECTED,
    USB_MIDI_CONNECTED
} usb_midi_state_t;

typedef struct {
    usb_driver_config_t driver_config;
    usb_midi_config_t config;

    SemaphoreHandle_t lock;
    usb_midi_state_t state;

    usb_host_client_handle_t client;
    usb_device_handle_t device;
    uint8_t device_address;
    const usb_device_desc_t *device_descriptor;
    const usb_intf_desc_t *interface;

    usb_midi_port_t in;
    usb_midi_port_t out;
    TaskHandle_t transfer_task;
    SemaphoreHandle_t transfer_lock;
} usb_midi_t;


void usb_midi_driver_task(void *arg);

esp_err_t usb_midi_init(const usb_midi_config_t *config, usb_midi_t *usb_midi);

esp_err_t usb_midi_send(usb_midi_t *usb_midi, const midi_message_t *message);
