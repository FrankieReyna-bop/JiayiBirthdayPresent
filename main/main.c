#include <stdio.h>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_lcd.h"
#include "esp_log.h"
#include "esp_sleep.h"

#define WATER_SENSOR_DIGITAL_GPIO GPIO_NUM_1  // Connect to "S" or DO pin
#define LCDADDR 0x27
#define COOLDOWN_SEC      (15)

SemaphoreHandle_t mutex; 
#include "esp_log.h"

gpio_config_t io_conf = {
    .pin_bit_mask = 1ULL << WATER_SENSOR_DIGITAL_GPIO,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};




char gca[40];

static const char *TAG = "WATER_SENSOR";

void app_main(void) {
    gpio_config(&io_conf);
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        i2c_master_init();
        lcd_init(LCDADDR);
        mutex = xSemaphoreCreateMutex();

        sprintf(gca, "Water Detected!");
        ESP_LOGI(TAG, "SENSOR_WAKEUP");

        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
            lcd_clear(LCDADDR);
            lcd_put_cursor(LCDADDR, 0, 0);
            lcd_send_string(LCDADDR, gca);
            vTaskDelay(pdMS_TO_TICKS(2000));
            sprintf(gca, "deep sleep int");
            lcd_clear(LCDADDR);
            lcd_send_string(LCDADDR, gca);
            vTaskDelay(pdMS_TO_TICKS(2000));
            xSemaphoreGive(mutex);
        }
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);  // disable GPIO
        esp_sleep_enable_timer_wakeup((uint64_t)COOLDOWN_SEC * 1000000ULL);
        ESP_LOGI(TAG, "Cooldown active: sleeping for 24h");
        esp_deep_sleep_start();
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Cooldown ended");

        esp_sleep_enable_ext1_wakeup(1ULL << WATER_SENSOR_DIGITAL_GPIO, ESP_EXT1_WAKEUP_ANY_LOW);
        ESP_LOGI(TAG, "Now waiting for sensor trigger...");
        esp_deep_sleep_start();
    } else {
        ESP_LOGI(TAG, "Power-on or reset. Setting up sensor wakeup.");

        // Fresh boot: enable GPIO wakeup
        esp_sleep_enable_ext1_wakeup(1ULL << WATER_SENSOR_DIGITAL_GPIO, ESP_EXT1_WAKEUP_ANY_LOW);

        ESP_LOGI(TAG, "Sleeping until sensor triggers...");
        esp_deep_sleep_start();
    }
}
    

