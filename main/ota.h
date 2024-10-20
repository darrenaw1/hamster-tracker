#ifndef OTA_H
#define OTA_H

#include "esp_err.h"

#define FIRMWARE_UPGRADE_URL "https://drive.google.com/u/0/uc?id=1dbGPJuEjVX-0-c1mLOa-2iWjl2lDcFTR&export=download"

void init_ota(void);
esp_err_t start_ota_update(void);

#endif // OTA_H