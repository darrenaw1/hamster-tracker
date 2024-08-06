#include "gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define PIR_INPUT GPIO_NUM_5 

static const char *TAG = "GPIO";
static volatile uint16_t pir_counter = 0;

void gpioInitialise()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIR_INPUT),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio_config(&io_conf);
}

int getPirLevel()
{
    return gpio_get_level(PIR_INPUT);
}

uint16_t getPirCounter()
{
    return pir_counter;
}

static void pir_task(void *pvParameter)
{
    int last_pir_state = 0;
    while (1)
    {
        int current_pir_state = getPirLevel();
        if (current_pir_state == 1 && last_pir_state == 0)
        {
            pir_counter++;
            ESP_LOGI(TAG, "PIR activated. Count: %d", pir_counter);
        }
        last_pir_state = current_pir_state;
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
    }
}

void startPirTask()
{
    xTaskCreate(&pir_task, "pir_task", 2048, NULL, 5, NULL);
}