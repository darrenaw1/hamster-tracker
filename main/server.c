#include "server.h"
#include "ota.h"
#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "SERVER";

esp_err_t on_default_url(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URL: %s", req->uri);
    char resp_str[1024];
    snprintf(resp_str, sizeof(resp_str),
        "<html><body>"
        "<h1>Hamster Check-in</h1>"
        "<p>PIR Sensor Activations: %d</p>"
        "<button id='updateBtn' onclick='checkAndUpdate()'>Check and Update Firmware</button>"
        "<p id='updateStatus'></p>"
        "<script>"
        "function checkAndUpdate() {"
        "  document.getElementById('updateBtn').disabled = true;"
        "  document.getElementById('updateStatus').innerText = 'Checking for update...';"
        "  fetch('/start_update')"
        "    .then(response => response.text())"
        "    .then(data => {"
        "      document.getElementById('updateStatus').innerText = data;"
        "    })"
        "    .catch(error => {"
        "      document.getElementById('updateStatus').innerText = 'Error: ' + error;"
        "      document.getElementById('updateBtn').disabled = false;"
        "    });"
        "}"
        "</script>"
        "</body></html>",
        getPirCounter());
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

esp_err_t on_start_update(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Starting OTA update process");
    esp_err_t ret = start_ota_update();
    if (ret == ESP_OK) {
        httpd_resp_sendstr(req, "OTA update process started. The device will restart if a new version is available.");
    } else {
        httpd_resp_sendstr(req, "Failed to start OTA update process.");
    }
    return ESP_OK;
}


void init_server()
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

void init_mdns()
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set("Hamster Check-in ESP32 Web Server"));

    ESP_LOGI(TAG, "mDNS hostname set to: %s.local", MDNS_HOSTNAME);
}