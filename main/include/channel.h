#pragma once

#include <esp_err.h>
#include <driver/gpio.h>
#include <dac.h>


typedef struct {
    gpio_num_t gate_pin;
    gpio_num_t trigger_pin;
    gpio_num_t dac_pin;
} channel_config_t;

typedef struct {
    channel_config_t config;
    dac_t dac;

    uint8_t note;
    uint16_t cents;
    uint8_t velocity;
    uint8_t gate;
    uint8_t trigger;
} channel_t;


esp_err_t channel_init(const channel_config_t *config, channel_t *channel);

esp_err_t channel_set_note(channel_t *channel, uint8_t note);
esp_err_t channel_set_note_cents(channel_t *channel, uint16_t cents);
esp_err_t channel_set_velocity(channel_t *channel, uint8_t velocity);
esp_err_t channel_set_gate(channel_t *channel, bool gate);
esp_err_t channel_set_trigger(channel_t *channel, bool trigger);
