#include "dac.h"
#include <string.h>
#include <esp_log.h>


static const char *TAG = "dac";

static dac_t *dac_list[DAC_MAX_DEVICES];
static esp_timer_handle_t dac_global_timer;


static uint16_t IRAM_ATTR dac_quantize(uint32_t value) {
    value += DAC_OVERSAMPLE_RESOLUTION / 2;
    return value & ~DAC_OVERSAMPLE_MASK;
}

static void IRAM_ATTR dac_timer_callback(void *arg) {
    dac_t *dac;

    // update the oversampling on all dacs
    for (int i = 0; i < DAC_MAX_DEVICES; i++) {
        dac = dac_list[i];
        if (dac == NULL) continue;

        // apply the quantization and store the error for the next iteration
        uint32_t value_a = dac->value_a + dac->error_a;
        uint32_t quant_a = dac_quantize(value_a);
        dac->error_a = value_a - quant_a;

        uint32_t value_b = dac->value_b + dac->error_b;
        uint32_t quant_b = dac_quantize(value_b);
        dac->error_b = value_b - quant_b;

        // write the values to the dac
        dac_write_raw(dac, DAC_A, (uint16_t) (quant_a >> DAC_OVERSAMPLE_BITS));
        dac_write_raw(dac, DAC_B, (uint16_t) (quant_b >> DAC_OVERSAMPLE_BITS));
    }
}

esp_err_t dac_global_init() {
    esp_err_t err;

    // reset the dac list
    memset(dac_list, 0, sizeof(dac_list));

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
    err = spi_bus_initialize(DAC_SPI_HOST, &bus_config, SPI_DMA_DISABLED);
    if (err != ESP_OK) return err;

    // setup the timer for oversampling
    const esp_timer_create_args_t timer_config = {
        .name = "dac_timer",
        .dispatch_method = ESP_TIMER_ISR,
        .callback = dac_timer_callback
    };

    ESP_LOGI(TAG, "initializing global timer");
    err = esp_timer_create(&timer_config, &dac_global_timer);
    if (err != ESP_OK) return err;

    err = esp_timer_start_periodic(dac_global_timer, DAC_TIMER_PERIOD_US);
    if (err != ESP_OK) return err;

    return ESP_OK;
}

esp_err_t dac_init(const dac_config_t *config, dac_t *dac) {
    esp_err_t err;

    // find an empty slot
    int dac_index;
    for (dac_index = 0; dac_index < DAC_MAX_DEVICES; dac_index++) {
        if (dac_list[dac_index] == NULL) break;
    }
    if (dac_index == DAC_MAX_DEVICES) {
        ESP_LOGE(TAG, "too many dacs");
        return ESP_ERR_INVALID_STATE;
    }

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

    // write the initial values
    dac_write_raw(dac, DAC_A, 0);
    dac_write_raw(dac, DAC_B, 0);

    // add the dac to the list so that the timer callback can update it
    dac_list[dac_index] = dac;

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
    // skip duplicate values
    if (channel == DAC_A) {
        if (value == dac->last_written_a) return ESP_OK;
        dac->last_written_a = value;
    } else {
        if (value == dac->last_written_b) return ESP_OK;
        dac->last_written_b = value;
    }

    // prepare the data packet
    uint16_t data = (channel << DAC_CHANNEL_OFFSET)
        | (DAC_UNBUFFERED << DAC_BUFFERED_OFFSET)
        | (DAC_GAIN_1 << DAC_GAIN_OFFSET)
        | (DAC_ACTIVE << DAC_SHUTDOWN_OFFSET)
        | ((value & DAC_DATA_MASK) << DAC_DATA_OFFSET);

    // prepare the transaction struct
    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
        .tx_data = { data >> 8, data & 0xff }
    };

    // send it via the spi interface
    return spi_device_polling_transmit(dac->spi_device, &trans);
}
