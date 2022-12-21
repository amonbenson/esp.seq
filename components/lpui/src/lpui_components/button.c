#include "lpui_components/button.h"

#include <esp_err.h>
#include <esp_check.h>


static const char *TAG = "button";


esp_err_t button_init(button_t *button, const button_config_t *config) {
    button->config = *config;
    
    // force a size of 1x1
    button->config.cmp_config.size = (lpui_size_t) { width: 1, height: 1 };

    const lpui_component_functions_t functions = {
        .context = button,
        .key_event = button_key_event
    };
    ESP_RETURN_ON_ERROR(lpui_component_init(&button->cmp, &button->config.cmp_config, &functions),
        TAG, "failed to initialize component");

    button->pressed = false;

    return ESP_OK;
}

esp_err_t button_key_event(void *context, const lpui_position_t pos, uint8_t velocity) {
    button_t *button = context;
    bool pressed = velocity > 0;

    // set the button according to the physical state
    if (button->config.mode == BUTTON_MODE_MOMENTARY) {
        ESP_RETURN_ON_ERROR(button_set_pressed(button, pressed),
            TAG, "failed to set button pressed");
    }

    // toggle if the physical button was pressed
    if (button->config.mode == BUTTON_MODE_TOGGLE && pressed) {
        ESP_RETURN_ON_ERROR(button_set_pressed(button, !button->pressed),
            TAG, "failed to set button pressed");
    }

    return ESP_OK;
}

esp_err_t button_draw(button_t *button) {
    lpui_t *ui = button->cmp.ui;

    // get the correct color
    lpui_color_t color = button->config.color;
    if (!button->pressed) color = lpui_color_darken(color);

    // draw the single pixel
    ESP_RETURN_ON_ERROR(lpui_sysex_reset(ui, LPUI_SYSEX_COMMAND_SET_LEDS),
        TAG, "failed to reset sysex");
    ESP_RETURN_ON_ERROR(lpui_sysex_add_led_color(ui, button->cmp.config.pos, color),
        TAG, "failed to add color to sysex");
    ESP_RETURN_ON_ERROR(lpui_sysex_commit(ui),
        TAG, "failed to commit sysex");
    
    return ESP_OK;
}

esp_err_t button_set_pressed(button_t *button, bool pressed) {
    if (button->pressed == pressed) return ESP_OK;
    button->pressed = pressed;

    // redraw the button
    ESP_RETURN_ON_ERROR(button_draw(button), TAG, "failed to draw button");

    // invoke the pressed callback
    if (button->pressed) {
        ESP_RETURN_ON_ERROR(CALLBACK_INVOKE(&button->config.callbacks, pressed, button),
            TAG, "failed to invoke pressed callback");
    } else {
        ESP_RETURN_ON_ERROR(CALLBACK_INVOKE(&button->config.callbacks, released, button),
            TAG, "failed to invoke released callback");
    }

    return ESP_OK;
}
