#include "ota.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_crt_bundle.h"
#include "esp_wifi.h"

#define TAG "OTA"

SemaphoreHandle_t ota_semaphore;

static int content_length = 0;
static int total_read = 0;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        content_length = 0;
        total_read = 0;
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        if (strcasecmp(evt->header_key, "Content-Length") == 0) {
            content_length = atoi(evt->header_value);
            ESP_LOGI(TAG, "Content-Length: %d", content_length);
        }
        break;
    case HTTP_EVENT_ON_DATA:
        total_read += evt->data_len;
        if (content_length > 0) {
            int progress = (total_read * 100) / content_length;
            ESP_LOGI(TAG, "Download progress: %d%%", progress);
        } else {
            ESP_LOGI(TAG, "Download progress: %d bytes", total_read);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        if (content_length > 0) {
            ESP_LOGI(TAG, "Download completed: %d bytes", total_read);
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    default:
        ESP_LOGI(TAG, "Unhandled HTTP Event: %d", evt->event_id);
        break;
    }
    return ESP_OK;
}

void run_ota(void *params)
{
    while (true)
    {
        xSemaphoreTake(ota_semaphore, portMAX_DELAY);
        ESP_LOGI(TAG, "Starting OTA update");

        // Ensure WiFi is connected
        if (esp_wifi_connect() != ESP_OK) {
            ESP_LOGE(TAG, "WiFi not connected. Cannot perform OTA.");
            continue;
        }

        esp_http_client_config_t config = {
            .url = "https://drive.usercontent.google.com/u/0/uc?id=15HOOG_H0MHGz2gEwEdUoHpOO4iajFnww&export=download",
            .crt_bundle_attach = esp_crt_bundle_attach,
            .skip_cert_common_name_check = true,
            .event_handler = _http_event_handler,
            .timeout_ms = 30000,  // Increase timeout to 30 seconds
        };

        esp_https_ota_config_t ota_config = {
            .http_config = &config,
        };

        esp_err_t ret = esp_https_ota(&ota_config);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "OTA update successful. Restarting...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
        }
    }
}

void otaStart(void *params)
{
    xSemaphoreGive(ota_semaphore);
}
