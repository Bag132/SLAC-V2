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
        printf("Turning on clock halt. Sending 0x%X\n", b);

    } else if (!halted && ch_on) {
        b &= 0x7F;
        if ((ret = reg_write(DS_ADDRESS, DS_REG_SECONDS, &b, 1)) != ESP_OK) {
            return ret;
        }
        printf("Turning off clock halt. Sending 0x%X\n", b);
    }

    // reg_read(DS_ADDRESS, DS_REG_SECONDS, &b, 1);
    // printf("Seconds register = 0x%X\n", b);

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

    
    // Fixme will clear clock halt
    esp_err_t ret;
    uint8_t sec_reg;
    if ((ret = reg_read(DS_ADDRESS, DS_REG_SECONDS, &sec_reg, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "Get Seconds error %d\n", sec);
        return ret;
    }

    uint8_t clock_halt = sec_reg & (1 << 7);
    sec |= clock_halt;

    if ((ret = reg_write(DS_ADDRESS, DS_REG_SECONDS, &sec, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "Seconds error %d\n", sec);
        return ret;
    }
    // printf("Set seconds %d\n", sec);
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
    h &= ~(1 << 6); // Set to 24 hour time

    esp_err_t ret;
    if ((ret = reg_write(DS_ADDRESS, DS_REG_HOURS, &h, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "Set hours error (0x%X)", ret);
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

void ds_log_time(const ds_time *dt)
{
    ESP_LOGI(TAG, "\n\tdt->seconds = %d\n\tdt->minutes = %d\n\tdt->hours = %d\n\tdt->day_of_week = %d\n\tdt->day_of_month = %d\n\tdt->months = %d\n\tdt->years = %d", 
            dt->seconds, dt->minutes, dt->hours, dt->day_of_week, dt->day_of_month, dt->months, dt->years);
}

struct tm ds_time_to_tm(const ds_time *d) 
{
    struct tm timeInfo;
    timeInfo.tm_sec = d->seconds;
    timeInfo.tm_min = d->minutes;
    timeInfo.tm_hour = d->hours;
    timeInfo.tm_wday = d->day_of_week - 1;
    timeInfo.tm_mday =  d->day_of_month - 1;
    timeInfo.tm_mon = d->months - 1;
    timeInfo.tm_year = d->years;

    return timeInfo;
}

ds_time tm_to_ds_time(const struct tm t) 
{
    ds_time d;
    d.seconds = t.tm_sec;
    d.minutes = t.tm_min;
    d.hours = t.tm_hour;
    d.day_of_week = t.tm_wday + 1;
    d.day_of_month = t.tm_mday + 1;
    d.months = t.tm_mon + 1;
    d.years = t.tm_year;

    return d;
}

time_t ds_compare_time(const ds_time *d1, const ds_time *d2)
{
    struct tm t1 = ds_time_to_tm(d1);
    struct tm t2 = ds_time_to_tm(d2);
    time_t t1_seconds = mktime(&t1);
    time_t t2_seconds = mktime(&t2);

    return t1_seconds - t2_seconds;
}

ds_time add_time(const ds_time *start_time, int days, int minutes, int seconds) 
{
    struct tm timeInfo;
    timeInfo.tm_sec = start_time->seconds;
    timeInfo.tm_min = start_time->minutes;
    timeInfo.tm_hour = start_time->hours;
    timeInfo.tm_wday = start_time->day_of_week - 1;
    timeInfo.tm_mday =  start_time->day_of_month - 1;
    timeInfo.tm_mon = start_time->months - 1;
    timeInfo.tm_year = start_time->years;

    seconds += minutes * 60 + days * 86400;

    time_t t = mktime(&timeInfo);  // Convert struct tm to time_t
    t += seconds;  // Add seconds
    timeInfo = *localtime(&t);  // Convert back to struct tm

    ds_time diff_time;
    diff_time.seconds = timeInfo.tm_sec;
    diff_time.minutes = timeInfo.tm_min;
    diff_time.hours = timeInfo.tm_hour;
    diff_time.day_of_week = timeInfo.tm_wday + 1;
    diff_time.day_of_month = timeInfo.tm_mday + 1;
    diff_time.months = timeInfo.tm_mon + 1;
    diff_time.years = timeInfo.tm_year;

    return diff_time;
}
