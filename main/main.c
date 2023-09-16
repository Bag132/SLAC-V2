#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "I2C_lib.h"
#include <time.h>
#include "DS1307.h"
#include "light_strip.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_app_trace.h"
#include <esp_timer.h>
#include "lcd_display.h"


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



// TODO: Web interface, connect to internet, flash lights
void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // ds_set_clock_halt(false);
    // ds_set_month(99);
    // ds_time dt;
    // ESP_ERROR_CHECK(ds_get_time(&dt));
    // ds_log_time(&dt);

    ls_setup(RED_GPIO_NUM, GREEN_GPIO_NUM, BLUE_GPIO_NUM);
    ls_color c = {.r = 0, .g = 255, .b = 255};
    ls_set_rgb(&c);

    // ledc_cbs_t cbs = {.fade_cb = &fade_callback};
    // ls_color color_1 = {.r = 0, .g = 255, .b = 0}, color_2 = {.r = 0, .g = 255, .b = 0};
    // ls_set_fade_callback(&cbs, (void*) &color_1);
    // c.r = 255;
    // c.b = 0;
    // ls_time_fade(&c, 1000);

    
    lcd_init();
    lcd_print("rich fonar");
    
    // while(1) {
    //     ls_set_rgb(&c);
    // }
    // ls_time_fade(0, 255, 0, 1000);
    
    
    ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    // ESP_LOGI(TAG, "I2C de-initialized successfully");
}


