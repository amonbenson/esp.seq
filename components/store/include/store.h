#pragma once

#include <esp_err.h>
#include <sdmmc_cmd.h>


#define STORE_SD_CS 34
#define STORE_SD_MOSI 35
#define STORE_SD_SCK 36
#define STORE_SD_MISO 37

#define STORE_SD_MOUNT_POINT "/sdcard"
#define STORE_SD_FORMAT_IF_MOUNT_FAILED true


typedef struct {
    sdmmc_card_t *sd_card;
} store_t;


esp_err_t store_init();
