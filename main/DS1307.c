#include "DS1307.h"
#include "I2C_lib.h"

static const char *TAG = "DS1307.c";

#define DS_ADDRESS      (0xD0 >> 1)
#define DS_REG_SECONDS  0x00
#define DS_REG_MINUTES  0x01
#define DS_REG_HOURS    0x02
#define DS_REG_DAY      0x03
#define DS_REG_DATE     0x04
#define DS_REG_MONTH    0x05
#define DS_REG_YEAR     0x06
#define DS_REG_CONTROL  0x07

esp_err_t ds_set_clock_halt(bool halted)
{
    uint8_t b;
    // Read seconds register
    esp_err_t ret;
    if ((ret = reg_read(DS_ADDRESS, DS_REG_SECONDS, &b, 1)) != ESP_OK) {
        return ret;
    }

    uint8_t ch_on = b & 0x80;

    if (halted && !ch_on) {
        b |= 0x80;
        if ((ret = reg_write(DS_ADDRESS, DS_REG_SECONDS, &b, 1)) != ESP_OK) {
            return ret;
        }
    } else if (!halted && ch_on) {
        b &= 0x7F;
        if ((ret = reg_write(DS_ADDRESS, DS_REG_SECONDS, &b, 1)) != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t ds_get_time(ds_time* rtc_time) 
{
    uint8_t rtc_data[7];
    esp_err_t ret;
    if ((ret = reg_read(DS_ADDRESS, DS_REG_SECONDS, rtc_data, 7)) != ESP_OK) {
        return ret;
    }

    // Get seconds and remove CH bit 7
    uint8_t seconds_tens = (rtc_data[0] & 0x70) >> 4;
    uint8_t seconds = (10 * seconds_tens) + ((rtc_data[0] & 0x7F) & 0x0F);

    // Get minutes
    uint8_t minutes_tens = (rtc_data[1] & 0x70) >> 4;
    uint8_t minutes = (10 * minutes_tens) + (rtc_data[1] & 0x0F);


    // Get hours
    uint8_t hours_reg = rtc_data[2];

    uint8_t hours;
    // If in AM/PM mode 
    if (hours_reg & (1 << 6)) {
        uint8_t hours_tens = (hours_reg & 0x10) >> 4;
        hours = (10 * hours_tens) + (hours_reg & 0x0F);
        if (hours_reg & (1 << 5)) {
            hours += 12;
        }
    } else {
        uint8_t hours_tens = (hours_reg & 0x30) >> 4;
        hours = (10 * hours_tens) + (hours_reg & 0x0F);
    }


    uint8_t days = rtc_data[3];
    uint8_t day_of_month_reg = rtc_data[4];
    uint8_t day_of_month_tens = (day_of_month_reg & 0x30) >> 4;
    uint8_t day_of_month = (10 * day_of_month_tens) + (day_of_month_reg & 0x0F);

    uint8_t months_reg = rtc_data[5];
    uint8_t months_tens = (months_reg & 0x10) >> 4;
    uint8_t months = (10 * months_tens) + (months_reg & 0x0F);

    uint8_t years_reg = rtc_data[6];

    uint8_t years_tens = (years_reg & 0xF0) >> 4;
    uint8_t years = (10 * years_tens) + (years_reg & 0x0F);

    rtc_time->seconds = seconds;
    rtc_time->minutes = minutes;
    rtc_time->hours = hours;
    rtc_time->day_of_week = days;
    rtc_time->day_of_month = day_of_month; 
    rtc_time->months = months;
    rtc_time->years = years;

    return ESP_OK;
}

esp_err_t ds_set_seconds(const uint8_t seconds) 
{
    uint8_t tens = (seconds / 10) << 4;
    uint8_t sec = tens | (seconds % 10);
    
    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_SECONDS, &sec, 1)) != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t ds_set_minutes(const uint8_t minutes) 
{
    uint8_t tens = (minutes / 10) << 4;
    uint8_t min = tens | (minutes % 10);

    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_MINUTES, &min, 1)) != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t ds_set_hours(const uint8_t hours)
{
    uint8_t tens = (hours / 10) << 4;
    uint8_t h = tens | (hours % 10);
    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_HOURS, &h, 1)) != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t ds_set_day_of_week(const uint8_t day_of_week)
{
    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_DAY, &day_of_week, 1)) != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t ds_set_day_of_month(const uint8_t day_of_month) 
{
    uint8_t tens = (day_of_month / 10) << 4;
    uint8_t dom = tens | (day_of_month % 10);

    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_DATE, &dom, 1)) != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t ds_set_month(const uint8_t month) 
{
    uint8_t tens = (month / 10) << 4;
    uint8_t mon = tens | (month % 10);

    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_MONTH, &mon, 1)) != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

// Years from 2000
esp_err_t ds_set_year(const uint8_t year) 
{
    uint8_t tens = (year / 10) << 4;
    uint8_t y = tens | (year % 10);

    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_YEAR, &y, 1)) != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

void ds_log_time(ds_time *dt)
{
    ESP_LOGI(TAG, "dt->seconds = %d", dt->seconds);
    ESP_LOGI(TAG, "dt->minutes = %d", dt->minutes);
    ESP_LOGI(TAG, "dt->hours = %d", dt->hours);
    ESP_LOGI(TAG, "dt->day_of_week = %d", dt->day_of_week);
    ESP_LOGI(TAG, "dt->day_of_month = %d", dt->day_of_month);
    ESP_LOGI(TAG, "dt->months = %d", dt->months);
    ESP_LOGI(TAG, "dt->years = %d", dt->years);
}
