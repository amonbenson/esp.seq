#pragma once
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_timer.h>


#define DAC_MAX_DEVICES 4

#define DAC_TIMER_PERIOD_US 500

#define DAC_SPI_HOST SPI2_HOST
#define DAC_SPI_SPEED_HZ 10000000
#define DAC_SPI_MODE 0b00

#define DAC_SPI_MISO_PIN GPIO_NUM_37
#define DAC_SPI_MOSI_PIN GPIO_NUM_35
#define DAC_SPI_CLK_PIN GPIO_NUM_36

#define DAC_CHANNEL_OFFSET 15

#define DAC_BUFFERED_OFFSET 14
#define DAC_UNBUFFERED 0
#define DAC_BUFFERED 1

#define DAC_GAIN_OFFSET 13
#define DAC_GAIN_2 0
#define DAC_GAIN_1 1

#define DAC_SHUTDOWN_OFFSET 12
#define DAC_SHUTDOWN 0
#define DAC_ACTIVE 1

#define DAC_DATA_BITS 8
#define DAC_DATA_OFFSET (12 - DAC_DATA_BITS)
#define DAC_DATA_RESOLUTION (1 << DAC_DATA_BITS)
#define DAC_DATA_MASK (DAC_DATA_RESOLUTION - 1)

#define DAC_OVERSAMPLE_BITS 2
#define DAC_OVERSAMPLE_RESOLUTION (1 << DAC_OVERSAMPLE_BITS)
#define DAC_OVERSAMPLE_MASK (DAC_OVERSAMPLE_RESOLUTION - 1)

#define DAC_TOTAL_BITS (DAC_DATA_BITS + DAC_OVERSAMPLE_BITS)
#define DAC_TOTAL_RESOLUTION (1 << DAC_TOTAL_BITS)
#define DAC_TOTAL_MASK (DAC_TOTAL_RESOLUTION - 1)


typedef enum {
    DAC_A = 0,
    DAC_B = 1
} dac_channel_t;


typedef struct {
    gpio_num_t cs_pin;
} dac_config_t;

typedef struct {
    dac_config_t config;
    spi_device_handle_t spi_device;
    int64_t last_time;

    uint32_t value_a, value_b;
    uint32_t error_a, error_b;
    uint16_t last_written_a, last_written_b;
} dac_t;


esp_err_t dac_global_init();

esp_err_t dac_create(const dac_config_t *config, dac_t *dac);

esp_err_t dac_set_value_mapped(dac_t *dac, dac_channel_t channel, uint32_t value, uint32_t max);

esp_err_t dac_set_value(dac_t *dac, dac_channel_t channel, uint32_t value);

esp_err_t dac_write_raw(dac_t *dac, dac_channel_t channel, uint16_t value);
