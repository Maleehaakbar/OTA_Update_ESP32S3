#include <stdio.h>
#include "max30102.h"
#include "i2c_interface.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include <esp_log.h>
#include "wifi_app.h"

const char* TAG_MAIN = "MAIN_APP";

static void i2c_task(void *arg)
{  
    max30102_data data;
    esp_err_t ret;
    uint32_t val;
    i2c_init();
    sensor_init(&data, Spo2_mode, sample_average_2);

    for(;;)
    {
        ret = sample_fetch_max30102(&data);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG_MAIN, "Fetch data successfully");
        }
     
        ret = get_channel(SENSOR_CHAN_IR,&data,&val);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG_MAIN, "Get value %lu", val);
        }

        else 
        {
            ESP_LOGE(TAG_MAIN, "Failed to get sensor value");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
      
}
void app_main(void)
{   
    esp_err_t ret;
   
     ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_AP_init();
    xTaskCreate(i2c_task, "i2c_task", 1024*4, NULL , 10, NULL);
}