#pragma once

#include <esp_err.h>
#include "midi_types.h"


typedef struct {
    uint8_t command;
    uint8_t channel;
    union {
        uint8_t body[2];
        struct {
            uint8_t note;
            uint8_t velocity;
        } note_off;
        struct {
            uint8_t note;
            uint8_t velocity;
        } note_on;
        struct {
            uint8_t note;
            uint8_t pressure;
        } poly_key_pressure;
        struct {
            uint8_t control;
            uint8_t value;
        } control_change;
        struct {
            uint8_t program;
        } program_change;
        struct {
            uint8_t pressure;
        } channel_pressure;
        struct {
            int16_t value;
        } pitch_bend;
        struct {
            size_t length;
            const uint8_t *data;
        } sysex;
        struct {
            uint8_t piece;
            uint8_t value;
            uint8_t rate;
        } tcqf;
        struct {
            uint16_t value;
        } song_position;
        struct {
            uint8_t value;
        } song_select;
    };
} __attribute__((packed)) midi_message_t;


esp_err_t midi_message_decode(const uint8_t *data, size_t length, midi_message_t *message);

esp_err_t midi_message_encode(const midi_message_t *message, uint8_t *data, size_t length);

void midi_message_print(const midi_message_t *message);
