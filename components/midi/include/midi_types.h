#pragma once


#define MIDI_COMMAND_IS_VALID(command) ((command) & 0x80 && (command) != 0xF4 && (command) != 0xF5)
#define MIDI_COMMAND_IS_CHANNEL_VOICE(command) (MIDI_COMMAND_IS_VALID(command) && (command) < 0xF0)
#define MIDI_COMMAND_IS_SYSTEM_COMMON(command) (MIDI_COMMAND_IS_VALID(command) && (command) >= 0xF0)


#define MIDI_COMMAND_NOTE_OFF 0x80
#define MIDI_COMMAND_NOTE_ON 0x90
#define MIDI_COMMAND_POLY_KEY_PRESSURE 0xA0
#define MIDI_COMMAND_CONTROL_CHANGE 0xB0
#define MIDI_COMMAND_PROGRAM_CHANGE 0xC0
#define MIDI_COMMAND_CHANNEL_PRESSURE 0xD0
#define MIDI_COMMAND_PITCH_BEND 0xE0
#define MIDI_COMMAND_SYSEX 0xF0
#define MIDI_COMMAND_TCQF 0xF1
#define MIDI_COMMAND_SONG_POSITION 0xF2
#define MIDI_COMMAND_SONG_SELECT 0xF3
#define MIDI_COMMAND_TUNE_REQUEST 0xF6
#define MIDI_COMMAND_SYSEX_END 0xF7
#define MIDI_COMMAND_CLOCK 0xF8

#define MIDI_TCQF_FRAME_LSB 0
#define MIDI_TCQF_FRAME_MSB 1
#define MIDI_TCQF_SECOND_LSB 2
#define MIDI_TCQF_SECOND_MSB 3
#define MIDI_TCQF_MINUTE_LSB 4
#define MIDI_TCQF_MINUTE_MSB 5
#define MIDI_TCQF_HOUR_LSB 6
#define MIDI_TCQF_RATE_HOUR_MSB 7

#define MIDI_TCQF_RATE_24 0
#define MIDI_TCQF_RATE_25 1
#define MIDI_TCQF_RATE_29_97 2
#define MIDI_TCQF_RATE_30 3


typedef uint8_t midi_command_t;
typedef uint8_t midi_channel_t;
typedef uint8_t midi_note_t;
typedef uint8_t midi_velocity_t;
typedef uint8_t midi_control_t;
