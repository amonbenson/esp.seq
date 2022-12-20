#include "lpui.h"

#include <stdlib.h>
#include <string.h>
#include <esp_check.h>
#include "utlist.h"


static const char *TAG = "lpui";

static const uint8_t lpui_sysex_header[] = { LPUI_SYSEX_HEADER };
//static const lpui_color_t lpui_color_patterns[] = { LPUI_COLOR_PATTERNS };


esp_err_t lpui_init(lpui_t *ui, const lpui_config_t *config) {
    // store the config
    ui->config = *config;

    // reset the components list
    ui->components = NULL;

    // allocate the sysex buffer
    ui->buffer = calloc(LPUI_SYSEX_BUFFER_SIZE, sizeof(uint8_t));
    if (ui->buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // store sysex header
    memcpy(ui->buffer, lpui_sysex_header, sizeof(lpui_sysex_header));

    ui->buffer_ptr = ui->buffer;
    return ESP_OK;
}

esp_err_t lpui_free(lpui_t *ui) {
    free(ui->buffer);
    return ESP_OK;
}

esp_err_t lpui_component_init(lpui_component_t *cmp,
        const lpui_component_config_t *config,
        const lpui_component_functions_t *functions) {
    cmp->config = *config;

    cmp->functions = *functions;

    cmp->ui = NULL;
    cmp->next = NULL;

    return ESP_OK;
}

esp_err_t lpui_add_component(lpui_t *ui, lpui_component_t *cmp) {
    // link the ui and component
    cmp->ui = ui;
    LL_PREPEND(ui->components, cmp);
    return ESP_OK;
}

esp_err_t lpui_remove_component(lpui_t *ui, lpui_component_t *cmp) {
    // unlink the ui and component
    LL_DELETE(ui->components, cmp);
    cmp->ui = NULL;
    return ESP_OK;
}


static bool lpui_sysex_buffer_has_space(lpui_t *ui, size_t size) {
    return ui->buffer_ptr + size < ui->buffer + LPUI_SYSEX_BUFFER_SIZE;
}

esp_err_t lpui_sysex_reset(lpui_t *ui, uint8_t command) {
    // reset the buffer pointer right after the header
    ui->buffer_ptr = ui->buffer + sizeof(lpui_sysex_header);

    // write the command
    *ui->buffer_ptr++ = command;
    return ESP_OK;
}

esp_err_t lpui_sysex_add_color(lpui_t *ui, const lpui_color_t color) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 3), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    // write rgb components
    *ui->buffer_ptr++ = color.red;
    *ui->buffer_ptr++ = color.green;
    *ui->buffer_ptr++ = color.blue;

    return ESP_OK;
}

esp_err_t lpui_sysex_add_led(lpui_t *ui, const lpui_position_t pos, const lpui_color_t color) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 4), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    // write the position and rgb components
    *ui->buffer_ptr++ = pos.x + pos.y * 10;
    *ui->buffer_ptr++ = color.red;
    *ui->buffer_ptr++ = color.green;
    *ui->buffer_ptr++ = color.blue;

    return ESP_OK;
}

esp_err_t lpui_sysex_commit(lpui_t *ui) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 1), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    // add the terminator byze and calculate the length
    *ui->buffer_ptr++ = 0xF7;
    size_t length = ui->buffer_ptr - ui->buffer;

    // invoke the sysex ready callback to render out the buffer
    return CALLBACK_INVOKE(&ui->config.callbacks, sysex_ready, ui, ui->buffer, length);
}


static bool lpui_component_contains_position(const lpui_component_t *cmp, const lpui_position_t pos) {
    return pos.x >= cmp->config.pos.x &&
        pos.x < cmp->config.pos.x + cmp->config.size.width &&
        pos.y >= cmp->config.pos.y &&
        pos.y < cmp->config.pos.y + cmp->config.size.height;
}

esp_err_t lpui_midi_recv(lpui_t *ui, const midi_message_t *message) {
    uint8_t note, velocity;

    // retrieve the note and velocity
    switch (message->command) {
        case MIDI_COMMAND_NOTE_ON:
            note = message->note_on.note;
            velocity = message->note_on.velocity;
            break;
        case MIDI_COMMAND_NOTE_OFF:
            note = message->note_off.note;
            velocity = 0;
            break;
        case MIDI_COMMAND_CONTROL_CHANGE:
            note = message->control_change.control;
            velocity = message->control_change.value;
            break;
        default:
            return ESP_OK;
    }

    // convert the note into a button
    lpui_position_t pos = {
        .x = note % 10,
        .y = note / 10
    };

    // traverse the components list and find the first one to handle the event
    for (lpui_component_t *cmp = ui->components; cmp != NULL; cmp = cmp->next) {
        // skip components that do not contain the position
        if (!lpui_component_contains_position(cmp, pos)) continue;

        // invoke the button_event callback
        ESP_RETURN_ON_ERROR(CALLBACK_INVOKE_REQUIRED(&cmp->functions, button_event, pos, velocity),
            TAG, "component does not implement button_event callback");
    }

    return ESP_OK;
}
