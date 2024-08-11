#ifndef SERVER_H
#define SERVER_H

#include "mdns.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "connect.h"
#include "ota.h"
#include "gpio.h"

#define CURRENT_VERSION 1

esp_err_t on_default_url(httpd_req_t *req);
esp_err_t on_check_update(httpd_req_t *req);
esp_err_t on_start_update(httpd_req_t *req);
void init_server();
void init_mdns();

#endif