#include "ota.h"
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"
#include "esp_ota_ops.h"

#define TAG "OTA"
SemaphoreHandle_t ota_semaphore;

extern const uint8_t server_cert_pem_start[] asm("_binary_google_cer_start");

esp_err_t client_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

esp_err_t validate_image_header(esp_app_desc_t *incoming_ota_desc)
{
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_app_desc_t running_partition_description;
    esp_ota_get_partition_description(running_partition, &running_partition_description);

    ESP_LOGI(TAG, "current version is %s", running_partition_description.version);
    ESP_LOGI(TAG, "new version is %s", incoming_ota_desc->version);

    if (strcmp(running_partition_description.version, incoming_ota_desc->version) == 0)
    {
        ESP_LOGW(TAG, "NEW VERSION IS THE SAME AS CURRENT VERSION. ABORTING");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void run_ota(void *params)
{
    while (true)
    {
        xSemaphoreTake(ota_semaphore, portMAX_DELAY);
        ESP_LOGI(TAG, "Invoking OTA");

        ESP_ERROR_CHECK(example_connect());

        esp_http_client_config_t clientConfig = {
            .url = FIRMWARE_UPGRADE_URL,
            .event_handler = client_event_handler,
            .cert_pem = (char *)server_cert_pem_start
        };

        esp_https_ota_config_t ota_config = {
            .http_config = &clientConfig
        };

        esp_https_ota_handle_t ota_handle = NULL;

        if (esp_https_ota_begin(&ota_config, &ota_handle) != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_https_ota_begin failed");
            example_disconnect();
            continue;
        }

        esp_app_desc_t incoming_ota_desc;
        if (esp_https_ota_get_img_desc(ota_handle, &incoming_ota_desc) != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed");
            esp_https_ota_finish(ota_handle);
            example_disconnect();
            continue;
        }

        if (validate_image_header(&incoming_ota_desc) != ESP_OK)
        {
            ESP_LOGE(TAG, "validate_image_header failed");
            esp_https_ota_finish(ota_handle);
            example_disconnect();
            continue;
        }

        while (true)
        {
            esp_err_t ota_result = esp_https_ota_perform(ota_handle);
            if (ota_result != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
                break;
        }

        if (esp_https_ota_finish(ota_handle) != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_https_ota_finish failed");
            example_disconnect();
            continue;
        }
        else
        {
            printf("restarting in 5 seconds\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            esp_restart();
        }
        ESP_LOGE(TAG, "Failed to update firmware");
    }
}

void init_ota(void)
{
    ota_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(run_ota, "run_ota", 1024 * 8, NULL, 2, NULL);
}

esp_err_t start_ota_update(void)
{
    if (ota_semaphore != NULL)
    {
        xSemaphoreGive(ota_semaphore);
        return ESP_OK;
    }
    return ESP_FAIL;
}