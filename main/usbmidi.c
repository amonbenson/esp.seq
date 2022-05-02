#import "usbmidi.h"
#include <usb/usb_host.h>


esp_err_t usbmidi_init() {
    esp_err_t err;

    // initialize the usb host stack
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };
    err = usb_host_install(&host_config);
    if (err != ESP_OK) return err;

    return ESP_OK;
}
