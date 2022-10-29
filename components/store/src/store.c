#include "store.h"


#include <esp_check.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>


static const char *TAG = "store";


static store_t store;


esp_err_t store_init() {
    // initialize the spi bus
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = STORE_SD_MOSI,
        .miso_io_num = STORE_SD_MISO,
        .sclk_io_num = STORE_SD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };
    
    ESP_LOGI(TAG, "SD card: initializing...");
    ESP_RETURN_ON_ERROR(spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA),
        TAG, "failed to initialize SPI bus");

    // mount the sd card
    const char mount_point[] = STORE_SD_MOUNT_POINT;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = STORE_SD_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = STORE_SD_FORMAT_IF_MOUNT_FAILED,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "SD card: mounting...");
    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &store.sd_card),
        TAG, "failed to mount SD card");
    ESP_LOGI(TAG, "SD card mounted.");

    return ESP_OK;
}
