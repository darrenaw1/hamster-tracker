#ifndef OTA_H
#define OTA_H

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

extern SemaphoreHandle_t ota_semaphore;

void otaStart(void *params);
void run_ota(void *params);
esp_err_t client_event_handler(esp_http_client_event_t *evt);

#endif
