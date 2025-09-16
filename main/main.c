#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "oled_anim.h"

#define WATER_SENSOR_DIGITAL_GPIO GPIO_NUM_5
#define COOLDOWN_SEC 15

static const char *TAG = "WATER_SENSOR";

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    // Configure GPIO for water sensor
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << WATER_SENSOR_DIGITAL_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Short delay to allow USB enumeration after deep sleep wakeup
    vTaskDelay(pdMS_TO_TICKS(50));

    // Force log level for safety

    // Identify wakeup reason
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_TIMER) {

        // Switch to sensor wakeup
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        esp_sleep_enable_ext1_wakeup(1ULL << WATER_SENSOR_DIGITAL_GPIO, ESP_EXT1_WAKEUP_ANY_LOW);

        ESP_LOGI(TAG, "Now waiting for sensor trigger...");
        printf("Hello world\n");
        fflush(stdout);   // ensures the host sees the message immediately

        vTaskDelay(pdMS_TO_TICKS(200)); // allow log flush
        esp_deep_sleep_start();

    } else if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        ESP_LOGI(TAG, "SENSOR_WAKEUP");
        printf("Hello 1world\n");
        fflush(stdout);   // ensures the host sees the message immediately

        // Re-init OLED every wakeup
        oled_anim_init();

        ESP_LOGI(TAG, "Playing animation...");
        oled_anim_play(100);

        // Flush logs and give OLED time to update
        vTaskDelay(pdMS_TO_TICKS(500));

        // Switch to cooldown timer
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
        esp_sleep_enable_timer_wakeup((uint64_t)COOLDOWN_SEC * 1000000ULL);

        ESP_LOGI(TAG, "Cooldown active: sleeping for %d sec", COOLDOWN_SEC);
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_deep_sleep_start();

    } else {
        ESP_LOGI(TAG, "Power-on or reset. Setting up sensor wakeup.");
        printf("Hello rworld\n");
        fflush(stdout);   // ensures the host sees the message immediately

        esp_sleep_enable_ext1_wakeup(1ULL << WATER_SENSOR_DIGITAL_GPIO, ESP_EXT1_WAKEUP_ANY_LOW);

        ESP_LOGI(TAG, "Sleeping until sensor triggers...");
        
        esp_deep_sleep_start();
    }
}
