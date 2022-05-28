#include "dac.h"
#include <freertos/task.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <freertos/semphr.h>


static const char *TAG = "dac";

static dac_t *dac_list[DAC_MAX_DEVICES];
//static esp_timer_handle_t dac_global_timer;
static SemaphoreHandle_t dac_lock, dac_start_lock;


/* static uint16_t IRAM_ATTR dac_quantize(uint32_t value) {
    value += DAC_OVERSAMPLE_RESOLUTION / 2;
    return value & ~DAC_OVERSAMPLE_MASK;
} */

static void dac_background_task() {
    dac_t *dac;

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

    ESP_ERROR_CHECK(spi_bus_initialize(DAC_SPI_HOST, &bus_config, SPI_DMA_DISABLED));

    // wait for the dacs to get started
    xSemaphoreTake(dac_start_lock, portMAX_DELAY);
    ESP_LOGI(TAG, "running on core %d", xPortGetCoreID());

    // main loop
    while (1) {
        // iterate through each dac
        for (int i = 0; i < DAC_MAX_DEVICES; i++) {
            dac = dac_list[i];
            if (dac == NULL) continue;

            // no oversampling
            if (DAC_OVERSAMPLE_BITS == 0) {
                dac_write_raw(dac, DAC_A, (uint16_t) dac->value_a);
                dac_write_raw(dac, DAC_B, (uint16_t) dac->value_b);
            } else {
                ESP_LOGW(TAG, "oversampling not implemented");

                // this is still buggy :(
                /*
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
                */
            }
        }

        vTaskDelay(DAC_TASK_DELAY_MS);
    }

    vTaskDelete(NULL);
}

esp_err_t dac_global_init() {
    dac_start_lock = xSemaphoreCreateBinary();
    dac_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(dac_lock);

    ESP_LOGI(TAG, "initializing background task");
    xTaskCreatePinnedToCore(dac_background_task, "dac_background_task", 2048, NULL, 5, NULL, DAC_TASK_CORE);

    /*
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
    */

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
    dac->config = *config;

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
    /* dac_write_raw(dac, DAC_A, 0);
    dac_write_raw(dac, DAC_B, 0); */

    // add the dac to the list so that the timer callback can update it
    dac_list[dac_index] = dac;

    return ESP_OK;
}

esp_err_t dac_start() {
    // start the background spi task
    xSemaphoreGive(dac_start_lock);

    return ESP_OK;
}

esp_err_t dac_set_value(dac_t *dac, dac_channel_t channel, uint32_t value) {
    xSemaphoreTake(dac_lock, portMAX_DELAY);

    if (channel == DAC_A) {
        dac->value_a = value;
    } else {
        dac->value_b = value;
    }

    xSemaphoreGive(dac_lock);
    return ESP_OK;
}

esp_err_t IRAM_ATTR dac_write_raw(dac_t *dac, dac_channel_t channel, uint16_t value) {
    esp_err_t ret;

    xSemaphoreTake(dac_lock, portMAX_DELAY);

    // skip duplicate values
    /* if (channel == DAC_A) {
        if (value == dac->last_written_a) {
            ret = ESP_OK;
            goto exit;
        }
        dac->last_written_a = value;
    } else {
        if (value == dac->last_written_b) {
            ret = ESP_OK;
            goto exit;
        }
        dac->last_written_b = value;
    } */

    // prepare the data packet
    uint16_t data = (channel << DAC_CHANNEL_OFFSET)
        | (DAC_UNBUFFERED << DAC_BUFFERED_OFFSET)
        | (DAC_GAIN_1 << DAC_GAIN_OFFSET)
        | (DAC_ACTIVE << DAC_SHUTDOWN_OFFSET)
        | ((value & DAC_DATA_MASK) << DAC_DATA_OFFSET);

    // prepare the transaction struct
    spi_transaction_t transaction = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
        .tx_data = { data >> 8, data & 0xff }
    };

    // send it via the spi interface
    ESP_GOTO_ON_ERROR(spi_device_queue_trans(dac->spi_device, &transaction, 0), exit,
        TAG, "sending transaction failed");
    
    xSemaphoreGive(dac_lock);

    // wait for the transaction to complete
    spi_transaction_t *result;
    ret = spi_device_get_trans_result(dac->spi_device, &result, portMAX_DELAY);

exit:
    xSemaphoreGive(dac_lock);
    return ret;
}
