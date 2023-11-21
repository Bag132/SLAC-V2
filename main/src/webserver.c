#include <string.h>

#include <freertos/FreeRTOS.h>
#include <esp_http_server.h>
#include <freertos/task.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include "esp_app_trace.h"
#include <sys/param.h>
#include <esp_wifi.h>
#include "light_strip.h"
#include <time.h>
#include <sys/time.h>
// #include "DS1307.h"
#include <esp_spiffs.h>
#include "lcd_display.h"
#include "alarm_clock.h"

#define WIFI_SSID "SLAC"
#define SYNC_RECV_LEN 22 + 1
#define SET_ALARM_LEN 5 + 1

/*
 * Serve OTA update portal (index.html)
 */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static const char *TAG = "webserver.c";

// FIXME: Years will stop working in 2255
void parse_sync_time(const char *input, ds_time *time)
{
	// 01-08-2023 18:59:54
	// const char *e = "01-08-2023 18:59:54 3";
    uint8_t days_tens = input[0] - 48;
	uint8_t days = (input[1] - 48) + (10 * days_tens);
	
	time->day_of_month = days;

	uint8_t months_tens = input[3] - 48;
	uint8_t months = (input[4] - 48) + (10 * months_tens) + 1;
	time->months = months;

	uint8_t years_tens = input[8] - 48;
	uint8_t years = (input[9] - 48) + (10 * years_tens);
	time->years = years;

	uint8_t hours_tens = input[11] - 48;
	uint8_t hours = (input[12] - 48) + (10 * hours_tens);
	time->hours = hours;

	uint8_t minutes_tens = input[14] - 48;
	uint8_t minutes = (input[15] - 48) + (10 * minutes_tens);
	time->minutes = minutes;

	uint8_t seconds_tens = input[17] - 48;
	uint8_t seconds = (input[18] - 48) + (10 * seconds_tens); 
	time->seconds = seconds;

	uint8_t day_of_week = input[20] - 48;
	time->day_of_week = day_of_week;
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
	char buf[SYNC_RECV_LEN];
	// esp_ota_handle_t ota_handle;
	int remaining = req->content_len;

	// const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
	// ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));
    
	int recv_len = httpd_req_recv(req, buf, MIN(remaining, SYNC_RECV_LEN));
	ds_time current_time = {0};
	if (recv_len != 50) {
		buf[recv_len] = '\0';
		const char *recv_str = buf;
		parse_sync_time(recv_str, &current_time);
		ESP_LOGI(TAG, "/sync recieved '%s'", recv_str);
	}

	// printf("Hours: %d, Minutes: %d, Seconds: %d\nDays: %d, Months: %d, Years: %d\nDay of week: %d\n", 
	// current_time.hours, current_time.minutes, current_time.seconds, current_time.day_of_month, current_time.months, current_time.years, current_time.day_of_week);

	// // Timeout Error: Just retry
	// if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
	// 	continue;

	// // Serious Error: Abort OTA
	// } else if (recv_len <= 0) {
	// 	httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Protocol Error");
	// 	return ESP_FAIL;
	// }


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
    // ls_set_rgb(&blue);
	// vTaskDelay(100 / portTICK_PERIOD_MS);
    // ls_set_rgb(&off);

	ESP_LOGI(TAG, "Setting DS1307 time");
	ds_log_time(&current_time);

	ESP_ERROR_CHECK(ds_set_hours(current_time.hours));
	ds_set_minutes(current_time.minutes);
	ds_set_seconds(current_time.seconds);
	ds_set_day_of_month(current_time.day_of_month);
	ds_set_day_of_week(current_time.day_of_week);
	ds_set_month(current_time.months);
	ds_set_year(current_time.years);

	ds_get_time(&current_time);
	ds_log_time(&current_time);
	
	lcd_display_time(&current_time);
	return ESP_OK;
}

esp_err_t set_alarm_post_handler(httpd_req_t *req)
{
    char buf[SET_ALARM_LEN];
	// esp_ota_handle_t ota_handle;
	int remaining = req->content_len;

	int recv_len = httpd_req_recv(req, buf, MIN(remaining, SET_ALARM_LEN));
	if (recv_len < 5) {
		ESP_LOGE(TAG, "/setalarm Recieved invalid data (<4)");
	}
	
	// 00:00
	buf[recv_len] = '\0';
	const char *recv_str = buf;
	ESP_LOGI(TAG, "/setalarm recieved '%s'", recv_str);
	
	uint8_t hours_tens = recv_str[0] - 48;
	uint8_t hours = (recv_str[1] - 48) + (10 * hours_tens);

	uint8_t minutes_tens = recv_str[3] - 48;
	uint8_t minutes = (recv_str[4] - 48) + (10 * minutes_tens);

	ds_time alarm_time = {0};
	alarm_time.hours = hours;
	alarm_time.minutes = minutes;
	// printf("Hours: %d, Minutes: %d\n", alarm_time.hours, alarm_time.minutes);

    ls_color purple = {.r = 255, .g = 0, .b = 255};
    ls_color off = {0};
    // ls_set_rgb(&purple);
	// vTaskDelay(100 / portTICK_PERIOD_MS);
    // ls_set_rgb(&off);

	FILE *time_file = fopen("/spiffs/alarm_time.txt", "w");
	if (time_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return 1;
    } else {
		fprintf(time_file, "%02d:%02d", alarm_time.hours, alarm_time.minutes);
		ESP_LOGI(TAG, "Wrote '%02d:%02d' to /spiffs/alarm_time.txt", alarm_time.hours, alarm_time.minutes);
		fclose(time_file);
		lcd_display_alarm_time();
	}

	alarm_set_alarm_time(alarm_time);
	
	return ESP_OK;
}

esp_err_t stop_alarm_post_handler(httpd_req_t *req) 
{
	ESP_LOGI(TAG, "Recieved POST stop alarm");
	alarm_stop();
	return ESP_OK;
}

uint8_t hex_char_to_decimal(char hex_char) {
	if (hex_char < 97) { // Regular number
		return hex_char - 48;
	} else { // Hex number
		return hex_char - 87;
	}
}

esp_err_t set_color_post_handler(httpd_req_t *req) 
{
	ESP_LOGI(TAG, "Recieved POST set color");

    char buf[255];
	// esp_ota_handle_t ota_handle;
	int remaining = req->content_len;

	int recv_len = httpd_req_recv(req, buf, MIN(remaining, 7));
	if (recv_len < 5) {
		ESP_LOGE(TAG, "/setalarm Recieved invalid data (<4)");
	}
	
	ESP_LOGI(TAG, "Recieved: '%s'", buf);

	
	uint8_t red_sixteens = hex_char_to_decimal(buf[1]) * 16;
	uint8_t red_ones = hex_char_to_decimal(buf[2]);
	uint8_t red = red_sixteens + red_ones;

	uint8_t green_sixteens = hex_char_to_decimal(buf[3]) * 16;
	uint8_t green_ones = hex_char_to_decimal(buf[4]);
	uint8_t green = green_sixteens + green_ones;

	uint8_t blue_sixteens = hex_char_to_decimal(buf[5]) * 16;
	uint8_t blue_ones = hex_char_to_decimal(buf[6]);
	uint8_t blue = blue_sixteens + blue_ones;
	ls_color c = {.r = red, .g = green, .b = blue};
	ls_set_rgb(&c);
	ESP_LOGI(TAG, "Red: %d", red);
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

httpd_uri_t stop_alarm_post = {
	.uri      = "/stopalarm",
	.method   = HTTP_POST,
	.handler  = stop_alarm_post_handler,
	.user_ctx = NULL
};

httpd_uri_t set_color_post = {
	.uri = "/setcolor",
	.method = HTTP_POST,
	.handler = set_color_post_handler,
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
		httpd_register_uri_handler(http_server, &stop_alarm_post);
		httpd_register_uri_handler(http_server, &set_color_post);
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
			.max_connection = 10
		},
	};

	res |= esp_wifi_set_mode(WIFI_MODE_AP);
	res |= esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
	res |= esp_wifi_start();

	return res;
}

void ws_run(void* a) {
	esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(softap_init());
	ESP_ERROR_CHECK(http_server_init());

	while(1) vTaskDelay(10);
}