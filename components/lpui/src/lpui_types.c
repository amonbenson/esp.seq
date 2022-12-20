#include "lpui_types.h"

#include <string.h>


inline lpui_color_t lpui_color_darken(lpui_color_t color) {
    return (lpui_color_t) {
        .red = color.red / 16,
        .green = color.green / 16,
        .blue = color.blue / 16,
    };
}

inline lpui_color_t lpui_color_lighten(lpui_color_t color) {
    return (lpui_color_t) {
        .red = color.red + (0xff - color.red) / 16,
        .green = color.green + (0xff - color.green) / 16,
        .blue = color.blue + (0xff - color.blue) / 16,
    };
}


void lpui_image_init(lpui_image_t *img) {
    // clear all buffers
    memset(&img->current, 0, sizeof(img->current));
    memset(&img->previous, 0, sizeof(img->previous));
}

void lpui_image_set_pixel(lpui_image_t *img, lpui_position_t pos, lpui_color_t color) {
    // set the pixel in the current buffer
    img->current.leds[pos.x][pos.y] = color;
}

lpui_color_t lpui_image_get_pixel(lpui_image_t *img, lpui_position_t pos) {
    // get the pixel from the current buffer
    return img->current.leds[pos.x][pos.y];
}

void lpui_image_update_previous(lpui_image_t *img) {
    // store the current buffer to the previous one
    memcpy(&img->previous, &img->current, sizeof(img->current));
}
