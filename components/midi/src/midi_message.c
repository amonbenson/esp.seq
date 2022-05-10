#include "midi_message.h"
#include <string.h>
#include <esp_log.h>


//static const char *TAG = "midi";


const uint8_t midi_channel_voice_message_lengths[7] = {
    3, 3, 3, 3, 2, 2, 3 // 0x80 - 0xE0
};

const uint8_t midi_system_common_message_lengths[8] = {
    2, 2, 3, 2, 1, 1, 1, 0 // 0xF0 - 0xF7
};


uint8_t midi_message_required_length(midi_command_t command) {
    if (MIDI_COMMAND_IS_CHANNEL_VOICE(command)) {
        return midi_channel_voice_message_lengths[(command & 0x70) >> 4];
    } else if (MIDI_COMMAND_IS_SYSTEM_COMMON(command)) {
        return midi_system_common_message_lengths[command & 0x07];
    } else {
        return 0; // invalid command
    }
}

esp_err_t midi_message_decode(const uint8_t *data, size_t length, midi_message_t *message) {
    midi_command_t command;
    midi_channel_t channel;
    uint8_t required_length, piece, value, rate;

    if (length < 1) return ESP_ERR_INVALID_SIZE;

    // validate the command
    command = data[0];
    if (!MIDI_COMMAND_IS_VALID(command)) {
        return ESP_ERR_INVALID_ARG;
    }

    // extract the channel
    if (MIDI_COMMAND_IS_CHANNEL_VOICE(command)) {
        channel = command & 0x0F;
        command = command & 0xF0;
    } else {
        channel = 0;
    }

    // validate the length
    required_length = midi_message_required_length(command);
    if (length < required_length) {
        return ESP_ERR_INVALID_SIZE;
    }

    // store the command and channel
    message->command = command;
    message->channel = channel;

    // store the message body
    switch (command) {
        case MIDI_COMMAND_TCQF:
            piece = (data[1] >> 4) & 0x07;
            value = data[1] & 0x0F;
            rate = 0;

            // extract the rate
            if (piece == MIDI_TCQF_RATE_HOUR_MSB) {
                rate = value >> 1;
                value = value & 0x01;
            }

            message->tcqf.piece = piece;
            message->tcqf.value = value;
            message->tcqf.rate = rate;
            break;
        case MIDI_COMMAND_PITCH_BEND:
            // align the pitch bend value manually
            message->pitch_bend.value = data[1] | (data[2] << 7);
            break;
        case MIDI_COMMAND_SYSEX:
            // validate the EOX byte
            if (data[length - 1] != MIDI_COMMAND_SYSEX_END) return ESP_ERR_INVALID_ARG;

            // store a pointer to the sysex data
            message->sysex.data = data;
            message->sysex.length = length;
            break;
        case MIDI_COMMAND_SYSEX_END:
            // single EOX byte is invalid
            return ESP_ERR_INVALID_ARG;
        default:
            // store the message body as is
            memcpy(message->body, data + 1, required_length - 1);
            break;
    }

    return ESP_OK;
}

esp_err_t midi_message_encode(const midi_message_t *message, uint8_t *data, size_t length) {
    midi_command_t command;
    midi_channel_t channel;
    uint8_t required_length, piece, value;

    command = message->command;
    channel = message->channel;

    // validate the command
    if (!MIDI_COMMAND_IS_VALID(command)) {
        return ESP_ERR_INVALID_ARG;
    }

    // validate the length
    required_length = midi_message_required_length(command);
    if (length < required_length) {
        return ESP_ERR_INVALID_SIZE;
    }

    // store the command
    if (MIDI_COMMAND_IS_CHANNEL_VOICE(command)) {
        command &= 0xF0;
        channel &= 0x0F;
        data[0] = command | channel;
    } else {
        data[0] = command;
    }

    // store the message body
    switch (command) {
        case MIDI_COMMAND_TCQF:
            piece = (message->tcqf.piece << 4) & 0x70;

            // mask out the value depending on the piece
            switch (piece) {
                case MIDI_TCQF_RATE_HOUR_MSB:
                    value = (message->tcqf.rate & 0x03) << 1 | (message->tcqf.value & 0x01);
                    break;
                case MIDI_TCQF_FRAME_MSB:
                    value = message->tcqf.value & 0x01;
                    break;
                case MIDI_TCQF_SECOND_MSB:
                case MIDI_TCQF_MINUTE_MSB:
                    value = message->tcqf.value & 0x03;
                    break;
                default:
                    value = message->tcqf.value & 0x07;
                    break;
            }

            data[1] = piece | value;

            break;
        case MIDI_COMMAND_PITCH_BEND:
            // separate the pitch bend value
            data[1] = message->pitch_bend.value & 0x7F;
            data[2] = (message->pitch_bend.value >> 7) & 0x7F;
            break;
        case MIDI_COMMAND_SYSEX:
            // validate the length
            if (length < message->sysex.length) {
                return ESP_ERR_INVALID_SIZE;
            }

            // store the sysex data
            memcpy(data, message->sysex.data, message->sysex.length);
            break;
        case MIDI_COMMAND_SYSEX_END:
            // single EOX byte is invalid
            return ESP_ERR_INVALID_ARG;
        default:
            // store the message body as is
            memcpy(data + 1, message->body, required_length - 1);
            break;
    }

    return ESP_OK;
}

void midi_message_print(const midi_message_t *message) {
    switch (message->command) {
        case MIDI_COMMAND_NOTE_OFF:
            printf("note off (note: %d, vel: %d)\n", message->note_off.note, message->note_off.velocity);
            break;
        case MIDI_COMMAND_NOTE_ON:
            printf("note on (note: %d, vel: %d)\n", message->note_on.note, message->note_on.velocity);
            break;
        case MIDI_COMMAND_POLY_KEY_PRESSURE:
            printf("poly key pressure (note: %d, pressure: %d)\n", message->poly_key_pressure.note, message->poly_key_pressure.pressure);
            break;
        case MIDI_COMMAND_CONTROL_CHANGE:
            printf("control change (control: %d, value: %d)\n", message->control_change.control, message->control_change.value);
            break;
        case MIDI_COMMAND_PROGRAM_CHANGE:
            printf("program change (program: %d)\n", message->program_change.program);
            break;
        case MIDI_COMMAND_CHANNEL_PRESSURE:
            printf("channel pressure (pressure: %d)\n", message->channel_pressure.pressure);
            break;
        case MIDI_COMMAND_PITCH_BEND:
            printf("pitch bend (value: %d)\n", message->pitch_bend.value);
            break;
        case MIDI_COMMAND_SYSEX:
            printf("sysex (");
            for (size_t i = 0; i < message->sysex.length; i++) {
                printf("%02x", message->sysex.data[i]);
                if (i < message->sysex.length - 1) printf(" ");
            }
            printf(")\n");
            break;
        case MIDI_COMMAND_TCQF:
            printf("tcqf (piece: %d, value: %d, rate: %d)\n", message->tcqf.piece, message->tcqf.value, message->tcqf.rate);
            break;
        default:
            printf("invalid command (%02x)\n", message->command);
            break;
    }
}
