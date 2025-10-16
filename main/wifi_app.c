#include "wifi_app.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "string.h"
#include "lwip/netdb.h"
#include "esp_event.h"
#include <esp_log.h>
#include "https_server.h"

const char* TAG_WIFI = "WIFI_AP";
esp_netif_t* esp_netif_ap  = NULL;

void wifi_event_handler(void* event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void* event_data)
{   if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
       wifi_event_ap_staconnected_t *info = (wifi_event_ap_staconnected_t*)event_data;
       ESP_LOGI(TAG_WIFI, "station "MACSTR" join, aid %d ", MAC2STR(info->mac) ,info->aid);
    }
    else if(event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
       wifi_event_ap_stadisconnected_t *disconnect_info = (wifi_event_ap_stadisconnected_t*)event_data;
       ESP_LOGI(TAG_WIFI, "station "MACSTR" disconncted, aid %d , reason %d", MAC2STR(disconnect_info->mac), disconnect_info->aid, disconnect_info->reason);

    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED)
    {
        ESP_LOGI(TAG_WIFI, "IP assigned to STA");
        start_server();
    }


}

void server_task()
{

}


void wifi_AP_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,NULL,NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,NULL,NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .password = WIFI_AP_PASSWORD,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .beacon_interval = WIFI_AP_BEACON_INTERVAL,
            .authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                    .required = false,
                    .capable =true,
            },
#ifdef CONFIG_ESP_WIFI_BSS_MAX_IDLE_SUPPORT
            .bss_max_idle_cfg = {
                .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                .protected_keep_alive = 1,
            },
#endif
        },
    };

    if (strlen(WIFI_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

    esp_netif_dhcps_stop(esp_netif_ap);
    inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);	
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);	
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);	
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));	
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
            WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
   
}