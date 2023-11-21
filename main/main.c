#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "I2C_lib.h"
#include <time.h>
#include "DS1307.h"
#include "alarm_clock.h"
#include "light_strip.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_app_trace.h"
#include <esp_timer.h>
#include "lcd_display.h"
#include "webserver.h"
#include <esp_spiffs.h>



static const char *TAG = "main";

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define RED_GPIO_NUM    32
#define GREEN_GPIO_NUM  33
#define BLUE_GPIO_NUM   23

#define MAX_DISTANCE_CM 500 // 5m max

#define TRIGGER_GPIO 5
#define ECHO_GPIO 18


// void ultrasonic_test(void *pvParameters)
// {
//     ultrasonic_sensor_t sensor = {
//         .trigger_pin = TRIGGER_GPIO,
//         .echo_pin = ECHO_GPIO
//     };

//     ultrasonic_init(&sensor);

//     while (true)
//     {
//         float distance;
//         esp_err_t res = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);
//         if (res != ESP_OK)
//         {
//             printf("Error %d: ", res);
//             switch (res)
//             {
//                 case ESP_ERR_ULTRASONIC_PING:
//                     printf("Cannot ping (device is in invalid state)\n");
//                     break;
//                 case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
//                     printf("Ping timeout (no device found)\n");
//                     break;
//                 case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
//                     printf("Echo timeout (i.e. distance too big)\n");
//                     break;
//                 default:
//                     printf("%s\n", esp_err_to_name(res));
//             }
//         }
//         else
//             printf("Distance: %0.04f cm\n", distance*100);

//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }


/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = 0,
        .scl_pullup_en = 0,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void led_triangle()
{
    int64_t start_time = esp_timer_get_time();
    const int64_t triangle_width_us = 2000000;
    float ratio = 255 / ((float) triangle_width_us / 2);
    float color_value;
    ls_color c;
    while(true) {
        int64_t current_time = esp_timer_get_time() - start_time;
        current_time %= triangle_width_us;
        if (current_time < triangle_width_us / 2) {
            color_value = (float) current_time * ratio;
        } else {
            color_value = ((float) ((triangle_width_us / 2) - (current_time - (triangle_width_us / 2)))) * ratio;
        }
        c.r = (uint8_t) color_value;
        c.g = 0;
        c.b = (uint8_t) color_value;
        ls_set_rgb(&c);
        
        // ESP_LOGI(TAG, "color_value = %f\n", color_value);
        // printf("color_value = %f\n", color_value);
    }

}

// Do some linked list fade chain if necessary
bool fade_callback(const ledc_cb_param_t *param, void *next_color)
{
    ls_color *c = (ls_color*) next_color;
    ls_time_fade(c, 1000);
    return false;
}

void setup_spiffs() 
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            } else {
                ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            }
        return;
    }
}

void display_time(void* a) 
{
    ds_time t;
    while(1) { // optimizr by only writibg to xhanged charater
        ds_get_time(&t);
        lcd_display_time(&t);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } 
}

// TODO: Web interface, connect to internet, flash lights
void app_main(void)
{
    // VL53LX_Dev_t t;
    // t.i2c_slave_address = 0x52;
    // t.comms_speed_khz = 100;
    // VL53LX_DEV tof;
    // tof->i2c_slave_address = 0x52;


    // VL53LX_WaitDeviceBooted(&tof);
    // VL53LX_DataInit(tof);
    
    // xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

    // printf("I2S PDM RX example start\n---------------------------\n");
    // xTaskCreate(i2s_example_pdm_rx_task, "i2s_example_pdm_rx_task", 4096, NULL, 5, NULL);

    // while(1)
    // vTaskDelay(5);

    setup_spiffs();
    ESP_ERROR_CHECK(i2c_master_init());

    ls_setup(RED_GPIO_NUM, GREEN_GPIO_NUM, BLUE_GPIO_NUM);


    ds_set_clock_halt(false);
    ds_time dt;
    ESP_ERROR_CHECK(ds_get_time(&dt));
    ds_log_time(&dt);
    ds_time tomorrow = add_time(&dt, 9, 0, 0);
    // ds_log_time(&tomorrow);

    ds_time alarm_time = {.hours=8, .minutes=2};
    ds_time next_alarm_time = get_next_alarm_time(&alarm_time);
    // ESP_LOGD("Next Alarm Time", TAG);
    ds_log_time(&next_alarm_time);

    void *param = NULL; 
    TaskHandle_t ws_task = NULL;
    xTaskCreate(ws_run, "WEBSERVER", 3584, param, 1, &ws_task);
    configASSERT(ws_task);

    lcd_init();
    lcd_display_alarm_time();

    TaskHandle_t clock_task = NULL;
    xTaskCreate(display_time, "CLOCK", 3584, param, 1, &clock_task);
    configASSERT(clock_task);

    TaskHandle_t alarm_task = NULL;
    xTaskCreate(alarm_run, "ALARM", 3584, param, 1, &alarm_task);
    configASSERT(alarm_task);


    // ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    // ESP_LOGI(TAG, "I2C de-initialized successfully");
    while(1)
    vTaskDelay(10);
}


