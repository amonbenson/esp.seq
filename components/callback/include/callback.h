#pragma once

#include <esp_err.h>


#define CALLBACK_TYPE(name) name ## _callback_t

#define CALLBACK_DECLARE(name, return_type, parameters...) \
    typedef return_type (*CALLBACK_TYPE(name))(void *context, ## parameters); \


#define CALLBACK_INVOKE(callback_group, name, parameters...) \
    (callback_group)->name \
        ? (callback_group)->name((callback_group)->context, ## parameters) \
        : ESP_OK

#define CALLBACK_INVOKE_REQUIRED(callback_group, name, parameters...) \
    (callback_group)->name \
        ? (callback_group)->name((callback_group)->context, ## parameters) \
        : ESP_FAIL

/* #define CALLBACK_INVOKE_CONTEXT(callback_group, name, context, parameters...) \
    (callback_group)->name \
        ? (callback_group)->name(context, ## parameters) \
        : ESP_FAIL */
