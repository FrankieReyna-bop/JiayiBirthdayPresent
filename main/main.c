#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "oled_anim.h"

#include <string.h> //for handling strings
#include "esp_system.h" //esp_init funtions esp_err_t 
#include "esp_wifi.h" //esp_wifi_init functions and wifi operations
#include "esp_event.h" //for wifi event
#include "nvs_flash.h" //non volatile storage
#include "lwip/err.h" //light weight ip packets error handling
#include "lwip/sys.h" //system applications for light weight ip apps

// Apparently an event handler, how do they get the event definitions though?
static EventGroupHandle_t s_wifi_event_group;
int retry_num = 0;
#define your_ssid "iPhone"
#define your_pass "amongus8"

#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
if(event_id == WIFI_EVENT_STA_START)
{
  printf("WIFI CONNECTING....\n");
}
else if (event_id == WIFI_EVENT_STA_CONNECTED)
{
  printf("WiFi CONNECTED\n");
  xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
}
else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
{
  printf("WiFi lost connection\n");
  if(retry_num<5){esp_wifi_connect();retry_num++;printf("Retrying to Connect...\n");}
}
else if (event_id == IP_EVENT_STA_GOT_IP)
{
  printf("Wifi got IP...\n\n");
}
}

void wifi_connection()
{   
    //Need nvs flash 
    nvs_flash_init();
     //                          s1.4
    // 2 - Wi-Fi Configuration Phase
    // First you initialized the network stack
    esp_netif_init();

    // Create default events like got IP so hander can recieve them
    esp_event_loop_create_default();     // event loop                    s1.2
    // Creates default station interface
    esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();

    //Initializes  Defualt drivers
    esp_wifi_init(&wifi_initiation); //     
    // Starts handler, attaches to different events
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    // Block creates config struct, thewn puts my ssid and pass in
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
            
           }
    
        };
    strcpy((char*)wifi_configuration.sta.ssid, your_ssid);
    strcpy((char*)wifi_configuration.sta.password, your_pass);    
    //Putss into wifi
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    // Puts to station
    esp_wifi_set_mode(WIFI_MODE_STA);
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
    printf( "wifi_init_softap finished. SSID:%s  password:%s",your_ssid, your_pass);
    
}

#define WATER_SENSOR_DIGITAL_GPIO GPIO_NUM_5
#define COOLDOWN_SEC 15

static const char *TAG = "WATER_SENSOR";

#include "esp_littlefs.h"
#include "esp_log.h"

#define LFS_MOUNT_POINT "/storage"

static const char *TAG1 = "littlefs_example";
static const char *TAG2 = "https_pull";

esp_vfs_littlefs_conf_t conf = {
    .base_path = LFS_MOUNT_POINT,
    .partition_label = "storage", // Use the name from your partitions.csv
    .format_if_mount_failed = true,
    .dont_mount = false,
};

void init_littlefs(void) {
    ESP_LOGE(TAG1, "Initializing LittleFS");

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG1, "Failed to mount or format filesystem");
        } else {
            ESP_LOGE(TAG1, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG1, "Failed to get LittleFS partition information");
    } else {
        ESP_LOGI(TAG1, "Partition size: total: %d, used: %d", total, used);
    }
}


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
        
        fflush(stdout);   // ensures the host sees the message immediately

        wifi_connection();
        init_littlefs();
        esp_vfs_littlefs_unregister("storage");

        // Re-init OLED every wakeup
        oled_anim_init();

        ESP_LOGI(TAG, "Playing animation...");
        oled_anim_play(100);
        ssd1306_clear();
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


