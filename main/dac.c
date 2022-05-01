#include "dac.h"
#include <string.h>
#include <esp_log.h>


static const char *TAG = "dac";


static uint16_t IRAM_ATTR dac_quantize(uint32_t value) {
    value += DAC_OVERSAMPLE_RESOLUTION / 2;
    return value & ~DAC_OVERSAMPLE_MASK;
}

static void IRAM_ATTR dac_timer_callback(void *arg) {
    dac_t *dac = (dac_t *) arg;

    // apply the quantization and store the error for the next iteration
    uint32_t value_a = dac->value_a + dac->error_a;
    uint32_t quant_a = dac_quantize(value_a);
    dac->error_a = value_a - quant_a;

    uint32_t value_b = dac->value_b + dac->error_b;
    uint32_t quant_b = dac_quantize(value_b);
    dac->error_b = value_b - quant_b;

    // write the values to the dac
    //ESP_LOGI(TAG, "value_a = %d, quant_a = %d, error_a = %d", value_a, quant_a, dac->error_a);
    //ESP_LOGI(TAG, "value_b = %d, quant_b = %d, error_b = %d", value_b, quant_b, dac->error_b);
    dac_write_raw(dac, DAC_A, (uint16_t) (quant_a >> DAC_OVERSAMPLE_BITS));
    dac_write_raw(dac, DAC_B, (uint16_t) (quant_b >> DAC_OVERSAMPLE_BITS));
}

esp_err_t dac_global_init() {
    // setup the spi bus
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

esp_err_t dac_create(const dac_config_t *config, dac_t *dac) {
    esp_err_t err;

    // store the config and set the initial values
    memset(dac, 0, sizeof(dac_t));
    memcpy(&dac->config, config, sizeof(dac_config_t));

    // initialize the spi device
    const spi_device_interface_config_t dev_config = {
        .clock_speed_hz = DAC_SPI_SPEED_HZ,
        .mode = DAC_SPI_MODE,
        .spics_io_num = config->cs_pin,
        .queue_size = 8
    };

    ESP_LOGI(TAG, "initializing SPI device (cs = %d)", config->cs_pin);
    err = spi_bus_add_device(DAC_SPI_HOST, &dev_config, &dac->spi_device);
    if (err != ESP_OK) return err;
    
    // setup the timer for oversampling
    const esp_timer_create_args_t timer_config = {
        .name = "dac_timer",
        .dispatch_method = ESP_TIMER_ISR,
        .callback = dac_timer_callback,
        .arg = (void *) dac
    };

    ESP_LOGI(TAG, "initializing timer");
    err = esp_timer_create(&timer_config, &dac->timer);
    if (err != ESP_OK) return err;

    err = esp_timer_start_periodic(dac->timer, DAC_TIMER_PERIOD_US);
    if (err != ESP_OK) return err;

    return ESP_OK;
}

esp_err_t dac_set_value_mapped(dac_t *dac, dac_channel_t channel, uint32_t value, uint32_t max) {
    // scale the value to the total resolution
    value = value * DAC_TOTAL_RESOLUTION / max;
    dac_set_value(dac, channel, value);

    return ESP_OK;
}

esp_err_t dac_set_value(dac_t *dac, dac_channel_t channel, uint32_t value) {
    if (channel == DAC_A) {
        dac->value_a = value;
    } else {
        dac->value_b = value;
    }

    return ESP_OK;
}

esp_err_t IRAM_ATTR dac_write_raw(dac_t *dac, dac_channel_t channel, uint16_t value) {
    uint16_t data = (channel << DAC_CHANNEL_OFFSET)
        | (DAC_UNBUFFERED << DAC_BUFFERED_OFFSET)
        | (DAC_GAIN_1 << DAC_GAIN_OFFSET)
        | (DAC_ACTIVE << DAC_SHUTDOWN_OFFSET)
        | ((value & DAC_DATA_MASK) << DAC_DATA_OFFSET);

    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
        .tx_data = { data >> 8, data & 0xff }
    };

    return spi_device_polling_transmit(dac->spi_device, &trans);
}
