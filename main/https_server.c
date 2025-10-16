#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lwip/ip4_addr.h"
#include "sys/param.h"
#include "freertos/FreeRTOS.h"
#include "https_server.h"
#include "tasks_config.h"
#include "wifi_app.h"
#include <string.h>

static const char* TAG_HTTPS = "HTTPS_SERVER";
static httpd_handle_t server_handle = NULL;
esp_timer_handle_t timer_handle = NULL;
uint64_t timeout_period = 8000000;

static uint8_t OTA_status = OTA_UPDATE_PENDING;

static httpd_handle_t https_server_configure(void);
static void fw_update_reset_callback(void *args);
static void firmware_reset_timer(void);

const esp_timer_create_args_t fw_update_reset_args = {
		.callback = &fw_update_reset_callback,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "fw_update_reset"
};

//Embedded files
extern const uint8_t jquery_3_3_1_min_js_start[]    asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[]      asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]				asm("_binary_index_html_start");
extern const uint8_t index_html_end[]				asm("_binary_index_html_end");
extern const uint8_t app_css_start[]				asm("_binary_app_css_start");
extern const uint8_t app_css_end[]					asm("_binary_app_css_end");
extern const uint8_t app_js_start[]					asm("_binary_app_js_start");
extern const uint8_t app_js_end[]					asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[]			asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]				asm("_binary_favicon_ico_end");

static esp_err_t jquery_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req,(const char*)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end-jquery_3_3_1_min_js_start );
    return ESP_OK;
}

static esp_err_t index_html_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req,(const char*)index_html_start, index_html_end-index_html_start);
    return ESP_OK;
}

static esp_err_t favicon_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req,(const char*)favicon_ico_start, favicon_ico_end-favicon_ico_start);
    return ESP_OK;
}

static esp_err_t app_css_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char*)app_css_start , app_css_end-app_css_start);
    return ESP_OK;
}

static esp_err_t app_js_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char*)app_js_start , app_js_end-app_js_start);
    return ESP_OK;
}

esp_err_t http_OTA_handler(httpd_req_t *req)
{
    int recv_len;
    int content_length = req->content_len;
    int content_received = 0;
    char ota_buffer[1024] ;
    bool is_req_body_started = false;
    bool flash_successful = false;
    esp_ota_handle_t ota_handle;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    do {

        recv_len = httpd_req_recv(req, ota_buffer , MIN(sizeof(ota_buffer), content_length));

        if (recv_len < 0)
        {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGE(TAG_HTTPS, "timeout error");
                continue;
            }

            ESP_LOGE(TAG_HTTPS, "HTTP server OTA error %d", recv_len);
            return ESP_FAIL;
        }
          
        ESP_LOGI(TAG_HTTPS, "OTA data received of length %d , total length %d", content_received , content_length);

        if(!is_req_body_started)
        {
            is_req_body_started = true;
            char* body_start = strstr(ota_buffer, "\r\n\r\n")+4;
            int body_len = recv_len -(body_start - ota_buffer);    //total length-header length

           esp_err_t err = esp_ota_begin(update_partition,OTA_SIZE_UNKNOWN,&ota_handle);

           if (err != ESP_OK)
           {
                ESP_LOGI(TAG_HTTPS,"http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
           }
           else
           {
                ESP_LOGI(TAG_HTTPS,"http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%lx\r\n", update_partition->subtype, update_partition->address);
                esp_ota_write(ota_handle,body_start,body_len);
                content_received += body_len;      //first time receive
           }
            
        }

        else
        {
            esp_ota_write(ota_handle,ota_buffer, recv_len);
            content_received += recv_len;
        }


    } while(recv_len > 0 && content_received < content_length);

    if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG_HTTPS, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%lx", boot_partition->subtype, boot_partition->address);
			flash_successful = true;
		}
		else
		{
			ESP_LOGI(TAG_HTTPS, "http_server_OTA_update_handler: FLASHED ERROR!!!");
		}
	}
	else
	{
		ESP_LOGI(TAG_HTTPS, "http_server_OTA_update_handler: esp_ota_end ERROR");
	}
    
    if(flash_successful)
    {
        ESP_LOGI(TAG_HTTPS,"OTA Update successful");
        OTA_status = OTA_UPDATE_SUCCESSFUL;
        firmware_reset_timer();
    }
    else
    {
        ESP_LOGI(TAG_HTTPS,"OTA Update failed");
    }

    return ESP_OK;
}

static void firmware_reset_timer(void)
{
    if (OTA_status == OTA_UPDATE_SUCCESSFUL)
    {
        ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args,&timer_handle));
        ESP_ERROR_CHECK(esp_timer_start_once(timer_handle, timeout_period));
    }
    else
	{
		ESP_LOGI(TAG_HTTPS, "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}

static void fw_update_reset_callback(void *args)
{
    ESP_LOGI(TAG_HTTPS, "fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}

esp_err_t OTA_status_handler(httpd_req_t *req)
{
    char otaJSON[100];

	ESP_LOGI(TAG_HTTPS, "OTAstatus requested");
    sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", OTA_status, __TIME__, __DATE__);
    httpd_resp_set_type(req , "application/json");
    httpd_resp_send(req , otaJSON,strlen(otaJSON));
    return ESP_OK;
}

esp_err_t max_sensor_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG_HTTPS, "/maxsensor.json requested");
    char sensorjson[100];
    uint32_t heart_rate_value;

    if(xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE)
    {
        heart_rate_value = sensor_read;
        xSemaphoreGive(xSemaphore);
    }
    
    else 
    {
        heart_rate_value = 0;
    }

    sprintf(sensorjson, "{\"Heart rate\": \"%lu\"}", heart_rate_value);
    httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, sensorjson, strlen(sensorjson));
    return ESP_OK;
}


static httpd_handle_t https_server_configure(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
    config.core_id = HTTP_SERVER_TASK_CORE_ID;
    config.task_priority = HTTP_SERVER_TASK_PRIORITY;
    config.max_uri_handlers = 20;
    config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

    ESP_LOGI(TAG_HTTPS, " Starting https server on port %d", config.server_port);
    
    if(httpd_start(&server_handle, &config) == ESP_OK)
    {
        ESP_LOGI(TAG_HTTPS, "registering URI handlers");
        httpd_uri_t jquery_min = 
        {
            .uri = "/jquery-3.3.1.min.js",
            .method = HTTP_GET,
            .handler = jquery_handler,
            .user_ctx = NULL

        };
        httpd_register_uri_handler(server_handle,&jquery_min);

        httpd_uri_t index_html =
        {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_html_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle,&index_html);
        
        httpd_uri_t favicon =
        {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = favicon_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle,&favicon);

        httpd_uri_t app_css =
        {
            .uri = "/app.css",
            .method = HTTP_GET,
            .handler = app_css_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle,&app_css);

        httpd_uri_t app_js =
        {
            .uri = "/app.js",
            .method = HTTP_GET,
            .handler = app_js_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle,&app_js);

        httpd_uri_t OTA_update = {
			.uri = "/OTAupdate",
			.method = HTTP_POST,
			.handler = http_OTA_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(server_handle, &OTA_update);

        httpd_uri_t OTA_status = {
            .uri = "/OTAstatus",
            .method = HTTP_POST,
            .handler = OTA_status_handler,
            .user_ctx = NULL

        };
        httpd_register_uri_handler(server_handle, &OTA_status);
        
        httpd_uri_t max_sensor = {
            .uri = "/maxsensor.json",
            .method = HTTP_GET,
            .handler = max_sensor_handler,
            .user_ctx = NULL

        };
        httpd_register_uri_handler(server_handle, &max_sensor);


        return server_handle;

    }

    ESP_LOGE(TAG_HTTPS, "error starting server");
    return NULL;

}



void start_server(void)
{
    if(server_handle == NULL)
    {
        server_handle = https_server_configure();
    }
}

void stop_server(void)
{
    if(server_handle)
    {
        httpd_stop(server_handle);
    }
}