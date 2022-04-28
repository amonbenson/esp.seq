#pragma once
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>


#define DAC_SPI_HOST SPI2_HOST
#define DAC_SPI_SPEED_HZ 10000000
#define DAC_SPI_MODE 0b00

#define DAC_SPI_MISO_PIN GPIO_NUM_37
#define DAC_SPI_MOSI_PIN GPIO_NUM_35
#define DAC_SPI_CLK_PIN GPIO_NUM_36

#define DAC_ADDRESS_OFFSET 15

#define DAC_BUFFERED_OFFSET 14
#define DAC_UNBUFFERED 0
#define DAC_BUFFERED 1

#define DAC_GAIN_OFFSET 13
#define DAC_DATA_MASK 0xff
#define DAC_GAIN_2 0
#define DAC_GAIN_1 1

#define DAC_SHUTDOWN_OFFSET 12
#define DAC_SHUTDOWN 0
#define DAC_ACTIVE 1

#define DAC_DATA_OFFSET 4


typedef enum {
    DAC_A = 0,
    DAC_B = 1
} dac_address_t;


typedef struct {
    gpio_num_t cs_pin;
    uint32_t vref_mv;
} dac_config_t;

typedef struct {
    dac_config_t config;
    spi_device_handle_t spi_device;
} dac_t;


esp_err_t dac_global_init();

esp_err_t dac_create(const dac_config_t *config, dac_t *device);

esp_err_t dac_write(dac_t *dac, dac_address_t addr, uint32_t value_mv);
