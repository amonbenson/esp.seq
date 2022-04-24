#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif


void app_main(void) {
    esp_err_t err;
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_BUILTIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    err = gpio_config(&io_conf);
    ESP_ERROR_CHECK(err);

    while (true) {
        gpio_set_level(LED_BUILTIN, 0);
        vTaskDelay(500 / portTICK_RATE_MS);
        gpio_set_level(LED_BUILTIN, 1);
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}
