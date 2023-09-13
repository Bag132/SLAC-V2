#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "I2C_lib.h"

typedef struct 
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week;
    uint8_t day_of_month;
    uint8_t months;
    uint8_t years;
} ds_time;

esp_err_t ds_set_clock_halt(bool halted);

esp_err_t ds_get_time(ds_time* rtc_time); 

esp_err_t ds_set_seconds(const uint8_t seconds);

esp_err_t ds_set_minutes(const uint8_t minutes);

esp_err_t ds_set_hours(const uint8_t hours);

esp_err_t ds_set_day_of_week(const uint8_t day_of_week);

esp_err_t ds_set_day_of_month(const uint8_t day_of_month);

esp_err_t ds_set_month(const uint8_t month);

esp_err_t ds_set_year(const uint8_t year);

void ds_log_time(ds_time *dt);
