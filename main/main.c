#include <stdio.h>
#include "connect.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "gpio.h"
#include "mdns.h"
#include "cJSON.h"
#include "ota.h"
#include "server.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "hamster-firmware-main-test");
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    ESP_ERROR_CHECK(wifi_connect_sta("EE-2HM2Z6", "d9q6fK6v3Ry9agDP", 10000));

    init_ota();
    init_server();
    init_mdns();

    while (1)
    {
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}