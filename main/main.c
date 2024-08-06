#include <stdio.h>
#include "connect.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "gpio.h"
#include "mdns.h"
#include "cJSON.h"
#include "ota.h"

#define CURRENT_VERSION 1
#define FIRMWARE_UPDATE_ENDPOINT "https://drive.usercontent.google.com/u/0/uc?id=1XiA16vt4sK-OGht0GS9UST4r4Ru3uG7x&export=download"

static const char *TAG = "SERVER";
#define MDNS_HOSTNAME "hamster-check-in"

/* this handles the webpage shown */
static esp_err_t on_default_url(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URL: %s", req->uri);
    char resp_str[1024];
    snprintf(resp_str, sizeof(resp_str),
        "<html><body>"
        "<h1>Hamster Check-in</h1>"
        "<p>PIR Sensor Activations: %d</p>"
        "<button id='updateBtn' onclick='startUpdate()'>Download Update</button>"
        "<script>"
        "function startUpdate() {"
        "  fetch('/start_update')"
        "    .then(response => response.text())"
        "    .then(data => {"
        "      alert(data);"
        "      document.getElementById('updateBtn').disabled = true;"
        "    });"
        "}"
        "</script>"
        "</body></html>",
        getPirCounter());
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t on_check_update(httpd_req_t *req)
{
    esp_http_client_config_t config = {
        .url = FIRMWARE_UPDATE_ENDPOINT,
        .method = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err;
    bool update_available = false;
    char version_str[16] = {0};

    if (esp_http_client_open(client, 0) == ESP_OK) {
        int content_length = esp_http_client_fetch_headers(client);
        if (content_length > 0 && content_length < 16) {
            int read_len = esp_http_client_read(client, version_str, content_length);
            if (read_len > 0) {
                int latest_version = atoi(version_str);
                update_available = (latest_version > CURRENT_VERSION);
            }
        }
    }
    esp_http_client_cleanup(client);

    char resp_str[64];
    snprintf(resp_str, sizeof(resp_str), "{\"update_available\": %s}", update_available ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t on_start_update(httpd_req_t *req)
{
    // Trigger the OTA update process
    otaStart(NULL);
    httpd_resp_sendstr(req, "Update process started. The device will restart if successful.");
    return ESP_OK;
}
static void init_server()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t default_url = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = on_default_url
    };
    httpd_register_uri_handler(server, &default_url);

    httpd_uri_t start_update_url = {
        .uri = "/start_update",
        .method = HTTP_GET,
        .handler = on_start_update
    };
    httpd_register_uri_handler(server, &start_update_url);
}

static void init_mdns()
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set("Hamster Check-in ESP32 Web Server"));

    ESP_LOGI(TAG, "mDNS hostname set to: %s.local", MDNS_HOSTNAME);
}

void app_main(void)
{
    ESP_LOGI(TAG, "new firmware test!");
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    ESP_ERROR_CHECK(wifi_connect_sta("EE-2HM2Z6", "d9q6fK6v3Ry9agDP", 10000));

    init_mdns();
    gpioInitialise();
    init_server();
    startPirTask();

    // Initialize OTA semaphore
    ota_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(run_ota, "run_ota", 8192, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}