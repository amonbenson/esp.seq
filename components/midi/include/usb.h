#pragma once

#include <esp_err.h>
#include <usb/usb_host.h>

typedef struct {
    TaskFunction_t task;
    void *arg;
} usb_driver_config_t;


esp_err_t usb_init(const usb_driver_config_t *driver);
