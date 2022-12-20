#pragma once

#include <esp_err.h>
#include "lpui_types.h"


#define LPUI_SYSEX_HEADER 0xF0, 0x00, 0x20, 0x29, 0x02, 0x10
#define LPUI_SYSEX_COMMAND_SET_LEDS 0x0B
#define LPUI_SYSEX_COMMAND_CLEAR 0x0E
#define LPUI_SYSEX_COMMAND_SET_GRID 0x0F


esp_err_t lpui_render(lpui_image_t *img, uint8_t *buf, uint16_t len);
