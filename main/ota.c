#include "ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <inttypes.h>
#include "esp_app_format.h"

static const char *TAG = "OTA";
static char ota_write_data[BUFFSIZE + 1] = { 0 };
static SemaphoreHandle_t ota_semaphore = NULL;

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void ota_task(void *pvParameter)
{
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    while (1) {
        xSemaphoreTake(ota_semaphore, portMAX_DELAY);
        ESP_LOGI(TAG, "Starting OTA update");

        esp_http_client_config_t config = {
            .url = FIRMWARE_UPGRADE_URL,
            .timeout_ms = 5000,
            .keep_alive_enable = true,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            continue;
        }

        err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            continue;
        }

        int content_length = esp_http_client_fetch_headers(client);
        ESP_LOGI(TAG, "Content-Length: %d", content_length);

        if (content_length <= 0) {
            ESP_LOGE(TAG, "Error: Server returned content length of %d", content_length);
            esp_http_client_cleanup(client);
            continue;
        }

        update_partition = esp_ota_get_next_update_partition(NULL);
        if (update_partition == NULL) {
            ESP_LOGE(TAG, "Failed to get next update partition");
            esp_http_client_cleanup(client);
            continue;
        }

        ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%" PRIx32,
                 update_partition->subtype, update_partition->address);

        err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            continue;
        }

        int binary_file_length = 0;
        while (1) {
            int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
            if (data_read < 0) {
                ESP_LOGE(TAG, "Error: Data read error");
                break;
            } else if (data_read > 0) {
                err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                    break;
                }
                binary_file_length += data_read;
                ESP_LOGI(TAG, "Written image length %d", binary_file_length);
            } else if (data_read == 0) {
                if (esp_http_client_is_complete_data_received(client) == true) {
                    ESP_LOGI(TAG, "Connection closed, all data received");
                    break;
                }
            }
        }

        ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);

        if (binary_file_length == 0) {
            ESP_LOGE(TAG, "No data was received from the server");
            esp_http_client_cleanup(client);
            esp_ota_abort(update_handle);
            continue;
        }

        err = esp_ota_end(update_handle);
        if (err != ESP_OK) {
            if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            } else {
                ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            }
            esp_http_client_cleanup(client);
            continue;
        }

        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            continue;
        }
        
        ESP_LOGI(TAG, "OTA update successful. Rebooting...");
        esp_http_client_cleanup(client);
        esp_restart();
    }
}

void init_ota(void)
{
    ota_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(ota_task, "ota_task", 8192, NULL, 5, NULL);
}

esp_err_t start_ota_update(void)
{
    if (ota_semaphore != NULL) {
        xSemaphoreGive(ota_semaphore);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t check_firmware_version(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    esp_http_client_config_t config = {
        .url = FIRMWARE_UPGRADE_URL,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    esp_http_client_fetch_headers(client);

    int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
    if (data_read >= sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
        const esp_app_desc_t *new_app_info = (esp_app_desc_t *) (ota_write_data + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t));
        ESP_LOGI(TAG, "New firmware version: %s", new_app_info->version);

        if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) != 0) {
            ESP_LOGI(TAG, "New version available");
            esp_http_client_cleanup(client);
            return ESP_OK;
        } else {
            ESP_LOGI(TAG, "Current running version is up to date");
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "Failed to read version info from server");
    }

    esp_http_client_cleanup(client);
    return ESP_FAIL;
}