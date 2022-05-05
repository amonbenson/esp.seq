#pragma once

#include <esp_err.h>
#include "midi_types.h"


typedef struct {
    midi_command_t command;
    midi_channel_t channel;
    union {
        uint8_t body[2];
        struct {
            midi_note_t note;
            midi_velocity_t velocity;
        } note_off;
        struct {
            midi_note_t note;
            midi_velocity_t velocity;
        } note_on;
        struct {
            midi_note_t note;
            midi_velocity_t pressure;
        } poly_key_pressure;
        struct {
            midi_control_t control;
            uint8_t value;
        } control_change;
        struct {
            uint8_t program;
        } program_change;
        struct {
            midi_velocity_t pressure;
        } channel_pressure;
        struct {
            int16_t value;
        } pitch_bend;
        struct {
            size_t length;
            const uint8_t *data;
        } sysex;
        struct {
            midi_tcqf_piece_t piece;
            uint8_t value;
            midi_tcqf_rate_t rate;
        } tcqf;
        struct {
            uint16_t value;
        } song_position;
        struct {
            uint8_t value;
        } song_select;
    };
} midi_message_t;


esp_err_t midi_message_decode(const uint8_t *data, size_t length, midi_message_t *message);

esp_err_t midi_message_encode(const midi_message_t *message, uint8_t *data, size_t length);

void midi_message_print(const midi_message_t *message);
