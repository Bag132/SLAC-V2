#include <string.h>

#include <freertos/FreeRTOS.h>
#include <esp_http_server.h>
#include <freertos/task.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_wifi.h>
#include "light_strip.h"
#include <time.h>
#include <sys/time.h>
#include "DS1307.h"

#define WIFI_SSID "SLAC"

/*
 * Serve OTA update portal (index.html)
 */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

// TODO: Years will stop working in 255
void parse_time(const char *input, ds_time *time)
{
	// 01-08-2023 18:59:54
	const char *e = "01-08-2023 18:59:54 3";
    uint8_t days_tens = input[0] - 48;
	uint8_t days = (input[1] - 48) + (10 * days_tens);

	uint8_t months_tens = input[3] - 48;
	uint8_t months = (input[4] - 48) + (10 * months_tens) + 1;

	uint8_t years_tens = input[8] - 48;
	uint8_t years = (input[9] - 48) + (10 * years_tens);

	uint8_t hours_tens = input[11] - 48;
	uint8_t hours = (input[12] - 48) + (10 * hours_tens);

	uint8_t minutes_tens = input[14] - 48;
	uint8_t minutes = (input[15] - 48) + (10 * minutes_tens);

	uint8_t seconds_tens = input[17] - 48;
	uint8_t seconds = (input[18] - 48) + (10 * seconds_tens); 

	uint8_t day_of_week = input[20] - 48;
}

esp_err_t index_get_handler(httpd_req_t *req)
{
	httpd_resp_send(req, (const char *) index_html_start, index_html_end - index_html_start);
	return ESP_OK;
}

/*
 * Handle OTA file upload
 */
esp_err_t sync_post_handler(httpd_req_t *req)
{
	char buf[50];
	// esp_ota_handle_t ota_handle;
	int remaining = req->content_len;

	// const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
	// ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));

    
	while (remaining > 0) {
		int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (recv_len != 50) {
            buf[recv_len] = '\0';
            const char *recv_str = buf;
            printf("%s\n", recv_str);
        }
        
		// // Timeout Error: Just retry
		// if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
		// 	continue;

		// // Serious Error: Abort OTA
		// } else if (recv_len <= 0) {
		// 	httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Protocol Error");
		// 	return ESP_FAIL;
		// }

		remaining -= recv_len;
	}

	// Validate and switch to new OTA image and reboot
	// if (esp_ota_end(ota_handle) != ESP_OK || esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
	// 		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation / Activation Error");
	// 		return ESP_FAIL;
	// }

	// httpd_resp_sendstr(req, "Firmware update complete, rebooting now!\n");

	// vTaskDelay(500 / portTICK_PERIOD_MS);
	// esp_restar=t();
    ls_color blue = {.r = 0, .g = 0, .b = 255};
    ls_color off = {0};
    ls_set_rgb(&blue);
	vTaskDelay(100 / portTICK_PERIOD_MS);
    ls_set_rgb(&off);

	return ESP_OK;
}

esp_err_t set_alarm_post_handler(httpd_req_t *req)
{
    	char buf[50];
	// esp_ota_handle_t ota_handle;
	int remaining = req->content_len;

	while (remaining > 0) {
		int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (recv_len != 50) {
            buf[recv_len] = '\0';
            const char *recv_str = buf;
            printf("%s\n", recv_str);
        }
		remaining -= recv_len;
	}

    ls_color purple = {.r = 255, .g = 0, .b = 255};
    ls_color off = {0};
    ls_set_rgb(&purple);
	vTaskDelay(100 / portTICK_PERIOD_MS);
    ls_set_rgb(&off);

	return ESP_OK;
}

/*
 * HTTP Server
 */
httpd_uri_t index_get = {
	.uri	  = "/",
	.method   = HTTP_GET,
	.handler  = index_get_handler,
	.user_ctx = NULL
};

httpd_uri_t sync_post = {
	.uri	  = "/sync",
	.method   = HTTP_POST,
	.handler  = sync_post_handler,
	.user_ctx = NULL
};

httpd_uri_t set_alarm_post = {
    .uri      = "/setalarm",
    .method   = HTTP_POST,
    .handler  = set_alarm_post_handler,
    .user_ctx = NULL
};

static esp_err_t http_server_init(void)
{
	static httpd_handle_t http_server = NULL;

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&http_server, &config) == ESP_OK) {
		httpd_register_uri_handler(http_server, &index_get);
		httpd_register_uri_handler(http_server, &sync_post);
        httpd_register_uri_handler(http_server, &set_alarm_post);
	}

	return http_server == NULL ? ESP_FAIL : ESP_OK;
}

/*
 * WiFi configuration
 */
static esp_err_t softap_init(void)
{
	esp_err_t res = ESP_OK;

	res |= esp_netif_init();
	res |= esp_event_loop_create_default();
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	res |= esp_wifi_init(&cfg);

	wifi_config_t wifi_config = {
		.ap = {
			.ssid = WIFI_SSID,
			.ssid_len = strlen(WIFI_SSID),
			.channel = 6,
			.authmode = WIFI_AUTH_OPEN,
			.max_connection = 3
		},
	};

	res |= esp_wifi_set_mode(WIFI_MODE_AP);
	res |= esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
	res |= esp_wifi_start();

	return res;
}

void ws_run(void) {
	esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(softap_init());
	ESP_ERROR_CHECK(http_server_init());

	/* Mark current app as valid */
	const esp_partition_t *partition = esp_ota_get_running_partition();
	printf("Currently running partition: %s\r\n", partition->label);

	esp_ota_img_states_t ota_state;
	if (esp_ota_get_state_partition(partition, &ota_state) == ESP_OK) {
		if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
			esp_ota_mark_app_valid_cancel_rollback();
		}
	}

	while(1) vTaskDelay(10);
}