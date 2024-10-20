#ifndef OTA_H
#define OTA_H

#include "esp_err.h"

#define FIRMWARE_UPGRADE_URL "https://drive.usercontent.google.com/u/0/uc?id=1mjbiB7Av9CwUsfri64gGh0DUyjoe4qx4&export=download"
#define BUFFSIZE 1024

void init_ota(void);
esp_err_t start_ota_update(void);
esp_err_t check_firmware_version(void);

#endif // OTA_H