#pragma once


#define MIDI_COMMAND_IS_VALID(command) ((command) & 0x80 && (command) != 0xF4 && (command) != 0xF5)
#define MIDI_COMMAND_IS_CHANNEL_VOICE(command) (MIDI_COMMAND_IS_VALID(command) && (command) < 0xF0)
#define MIDI_COMMAND_IS_SYSTEM_COMMON(command) (MIDI_COMMAND_IS_VALID(command) && (command) >= 0xF0)

#define MIDI_INVOKE_CALLBACK(callbacks, name, ...) \
    if ((callbacks)->name != NULL) { \
        (callbacks)->name(__VA_ARGS__); \
    }


typedef enum {
    MIDI_COMMAND_NOTE_OFF = 0x80,
    MIDI_COMMAND_NOTE_ON = 0x90,
    MIDI_COMMAND_POLY_KEY_PRESSURE = 0xA0,
    MIDI_COMMAND_CONTROL_CHANGE = 0xB0,
    MIDI_COMMAND_PROGRAM_CHANGE = 0xC0,
    MIDI_COMMAND_CHANNEL_PRESSURE = 0xD0,
    MIDI_COMMAND_PITCH_BEND = 0xE0,
    MIDI_COMMAND_SYSEX = 0xF0,
    MIDI_COMMAND_TCQF = 0xF1,
    MIDI_COMMAND_SONG_POSITION = 0xF2,
    MIDI_COMMAND_SONG_SELECT = 0xF3,
    MIDI_COMMAND_TUNE_REQUEST = 0xF6,
    MIDI_COMMAND_SYSEX_END = 0xF7,
    MIDI_COMMAND_CLOCK = 0xF8,
} midi_command_t;

typedef enum {
    MIDI_TCQF_FRAME_LSB = 0,
    MIDI_TCQF_FRAME_MSB = 1,
    MIDI_TCQF_SECOND_LSB = 2,
    MIDI_TCQF_SECOND_MSB = 3,
    MIDI_TCQF_MINUTE_LSB = 4,
    MIDI_TCQF_MINUTE_MSB = 5,
    MIDI_TCQF_HOUR_LSB = 6,
    MIDI_TCQF_RATE_HOUR_MSB = 7
} midi_tcqf_piece_t;

typedef enum {
    MIDI_TCQF_RATE_24 = 0,
    MIDI_TCQF_RATE_25 = 1,
    MIDI_TCQF_RATE_29_97 = 2,
    MIDI_TCQF_RATE_30 = 3
} midi_tcqf_rate_t;


typedef uint8_t midi_channel_t;
typedef uint8_t midi_note_t;
typedef uint8_t midi_velocity_t;
typedef uint8_t midi_control_t;
