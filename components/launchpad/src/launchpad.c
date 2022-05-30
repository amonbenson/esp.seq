#include "Launchpad.h"
#include <esp_err.h>
#include <esp_check.h>

#define LAUNCHPAD_VALIDATE_CONNECTED(launchpad) \
    ESP_RETURN_ON_FALSE(launchpad->connected, ESP_ERR_INVALID_STATE, \
        TAG, "not connected");

static const uint8_t launchpad_sysex_clear[] = { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x10, 0x0E, 0x00, 0xF7 };

static const char *TAG = "launchpad";


esp_err_t launchpad_pro_clear(launchpad_t *launchpad) {
    LAUNCHPAD_VALIDATE_CONNECTED(launchpad);

    return usb_midi_send_sysex(launchpad->config.usb_midi, launchpad_sysex_clear, sizeof(launchpad_sysex_clear));
}

esp_err_t launchpad_connected_callback(launchpad_t *launchpad, const usb_device_desc_t *desc) {
    ESP_RETURN_ON_FALSE(launchpad->connected == false, ESP_ERR_INVALID_STATE,
        TAG, "launchpad already connected");

    // check the device descriptor
    ESP_RETURN_ON_FALSE(desc->idVendor == LAUNCHPAD_VENDOR_ID, ESP_ERR_NOT_SUPPORTED,
        TAG, "unsupported vendor id 0x%04x", desc->idVendor);
    ESP_RETURN_ON_FALSE(desc->idProduct == LAUNCHPAD_PRO_PRODUCT_ID, ESP_ERR_NOT_SUPPORTED,
        TAG, "unsupported product id 0x%04x", desc->idProduct);

    ESP_LOGI(TAG, "Launchpad Pro connected");
    launchpad->connected = true;

    // clear the screen
    launchpad_pro_clear(launchpad);

    return ESP_OK;
}

esp_err_t launchpad_disconnected_callback(launchpad_t *launchpad) {
    LAUNCHPAD_VALIDATE_CONNECTED(launchpad);

    ESP_LOGI(TAG, "Launchpad Pro disconnected");
    launchpad->connected = false;

    return ESP_OK;
}

esp_err_t launchpad_recv_callback(launchpad_t *launchpad, const midi_message_t *message) {
    LAUNCHPAD_VALIDATE_CONNECTED(launchpad);

    //ESP_LOGI(TAG, "Launchpad Pro received message");
    //midi_message_print(message);

    return ESP_OK;
}

esp_err_t launchpad_init(const launchpad_config_t *config, launchpad_t *launchpad) {
    launchpad->config = *config;
    launchpad->connected = false;

    return ESP_OK;
}
