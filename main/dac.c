#include "dac.h"
#include <string.h>
#include <esp_log.h>


static const char *TAG = "dac";


esp_err_t dac_global_init() {
    const spi_bus_config_t bus_config = {
        .miso_io_num = DAC_SPI_MISO_PIN,
        .mosi_io_num = DAC_SPI_MOSI_PIN,
        .sclk_io_num = DAC_SPI_CLK_PIN,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 2
    };

    ESP_LOGI(TAG, "initializing SPI bus");
    return spi_bus_initialize(DAC_SPI_HOST, &bus_config, SPI_DMA_DISABLED);
}

esp_err_t dac_create(const dac_config_t *config, dac_t *device) {
    // store the config
    memcpy(&device->config, config, sizeof(dac_config_t));

    const spi_device_interface_config_t dev_config = {
        .clock_speed_hz = DAC_SPI_SPEED_HZ,
        .mode = DAC_SPI_MODE,
        .spics_io_num = config->cs_pin,
        .queue_size = 8
    };

    ESP_LOGI(TAG, "initializing SPI device (cs = %d)", config->cs_pin);
    return spi_bus_add_device(DAC_SPI_HOST, &dev_config, &device->spi_device);
}

esp_err_t dac_write(dac_t *dac, dac_address_t addr, uint32_t value_mv) {
    // calculate the output value based on the stored reference voltage
    uint32_t value = value_mv * 255 / dac->config.vref_mv;

    uint16_t data = (addr << DAC_ADDRESS_OFFSET)
        | (DAC_GAIN_1 << DAC_GAIN_OFFSET)
        | (DAC_ACTIVE << DAC_SHUTDOWN_OFFSET)
        | ((value & DAC_DATA_MASK) << DAC_DATA_OFFSET);

    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
        .tx_data = { data >> 8, data & 0xff }
    };

    ESP_LOGV(TAG, "writing %d", value);
    return spi_device_polling_transmit(dac->spi_device, &trans);
}
