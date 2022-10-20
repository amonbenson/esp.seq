#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <output.h>


static const char *TAG = "espmidi";


static const output_pins_t output_digital_pins = {
    .length = 2,
    .pins = { 4, 5 }
};
static const output_pins_t output_analog_pins = {
    .length = 2,
    .pins = { 2, 3 }
};
static const output_config_t output_config = {
    .digital_pins = &output_digital_pins,
    .analog_pins = &output_analog_pins
};


static output_t output;


void app_main(void) {
    ESP_LOGI(TAG, "ESP MIDI");

    // setup the pwm output unit
    output_init(&output, &output_config);

    vTaskDelete(NULL);
}
