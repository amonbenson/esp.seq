#pragma once

#include <stdint.h>


typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} lpui_color_t;

typedef struct {
    uint8_t x;
    uint8_t y;
} lpui_position_t;

typedef struct {
    uint8_t width;
    uint8_t height;
} lpui_size_t;

typedef struct {
    lpui_color_t leds[10][10];
    lpui_color_t side_led;
} lpui_image_buffer_t;

typedef struct {
    lpui_image_buffer_t current;
    lpui_image_buffer_t previous;
} lpui_image_t;


lpui_color_t lpui_color_darken(lpui_color_t color);
lpui_color_t lpui_color_lighten(lpui_color_t color);

void lpui_image_init(lpui_image_t *img);

void lpui_image_set_pixel(lpui_image_t *img, lpui_position_t pos, lpui_color_t color);
lpui_color_t lpui_image_get_pixel(lpui_image_t *img, lpui_position_t pos);

void lpui_image_update_previous(lpui_image_t *img);
