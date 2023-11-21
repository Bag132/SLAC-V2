#include "DS1307.h"
#include "esp_log.h"
#include "esp_app_trace.h"
#include "light_strip.h"

#define ALARM_LENGTH_MIN 2

const char *TAG = "alam_clock";

ds_time alarm_time = {0};
ds_time next_alarm_time = {0};
bool stop_requested = false;

typedef enum 
{
    ACTIVATED,
    DEACTIVATED,
    ALARMING,
    SNOOZING // lol
} alarm_state;

ds_time get_next_alarm_time(const ds_time *alarm_time)
{
    ds_time current_time;
    ESP_ERROR_CHECK(ds_get_time(&current_time));

    if (current_time.hours > alarm_time->hours || (current_time.hours == alarm_time->hours && current_time.minutes > alarm_time->minutes)) {
        ds_time tomorrow = add_time(&current_time, 1, 0, 0);
        tomorrow.hours = alarm_time->hours;
        tomorrow.minutes = alarm_time->minutes;
        tomorrow.seconds = 0;
        return tomorrow;
    } else if (current_time.hours <= alarm_time->hours) {
        ds_time next_alarm_time;
        if (current_time.minutes >= alarm_time->minutes && current_time.hours == alarm_time->hours) {
            next_alarm_time = add_time(&current_time, 1, 0, 0);
        } else {
            next_alarm_time = current_time;
            next_alarm_time.hours = alarm_time->hours;
            next_alarm_time.minutes = alarm_time->minutes;
            next_alarm_time.seconds = 0;
        }
        return next_alarm_time;
    }
    return current_time;
}

bool alarm_equals(ds_time *t1, ds_time *t2)
{
    return t1->day_of_month == t2->day_of_month && t1->hours == t2->hours && t1->minutes == t2->minutes;
}

ds_time read_alarm_time()
{
    ds_time null_time = {0};
    FILE *time_file = fopen("/spiffs/alarm_time.txt", "r");
    if (time_file == NULL) {
        ESP_LOGE(TAG, "Couldn't open alarm_time.txt");
        return null_time;
    }

    char time_str[20];
    time_str[5] = '\0';

    int n = fread(time_str, sizeof(char), 5, time_file);
    fclose(time_file);

    if (n != 5) {
        ESP_LOGE(TAG, "Invalid time in alarm_time.txt '%s'", time_str);
        return null_time;
    } else {
        uint8_t hours_tens = time_str[0] - 48;
        uint8_t hours = (time_str[1] - 48) + (10 * hours_tens);
        uint8_t minutes_tens = time_str[3] - 48;
        uint8_t minutes = (time_str[4] - 48) + (10 * minutes_tens);

        ds_time alarm_time = {.hours = hours, .minutes = minutes};
        return alarm_time;
    }

}

void alarm_set_alarm_time(ds_time t) {
    alarm_time = t;
    next_alarm_time = get_next_alarm_time(&alarm_time);
}

void alarm_stop() {
    stop_requested = true;
}

void alarm_run() {
    alarm_state current_state;
    current_state = ACTIVATED;

    bool blinking = false;
    bool firstActivate = true;
    bool firstDeactivate = true;
    bool firstAlarm = true;
    bool firstSnooze = true;
    
    alarm_time = read_alarm_time();
    next_alarm_time = get_next_alarm_time(&alarm_time);
    ds_time now;
    ds_get_time(&now);

    ds_time alarm_end = add_time(&now, 4, 0, 0);
    
    const ls_color alarm_color = {.r = 255, .g = 0, .b = 0};
    const ls_color blue = {.r = 0, .g = 0, .b = 255};
    const ls_color white = {.r = 255, .g = 255, .b = 255};
    const ls_color off_color = {0};

    while(1) {
        ds_get_time(&now);

        switch (current_state) {
            case ACTIVATED:
                blinking = false;
                firstDeactivate = true;
                firstAlarm = true;
                firstSnooze = true;
                stop_requested = false;

                if (firstActivate) {
                    ESP_LOGI(TAG, "Alarm State: Activated");
                    ls_set_rgb(&off_color);
                    firstActivate = false;
                }

                if (alarm_equals(&next_alarm_time, &now)) {
                    current_state = ALARMING;
                }

                if (false) {
                    current_state = DEACTIVATED;
                }
                break;
            case DEACTIVATED:
                blinking = false;
                firstActivate = true;
                firstAlarm = true;
                firstSnooze = true;

                if (firstDeactivate) {
                    ESP_LOGI(TAG, "Alarm State: Deactivated");
                    ls_set_rgb(&white);
                    firstDeactivate = false;
                }

                if (stop_requested) {     
                    next_alarm_time = get_next_alarm_time(&alarm_time);
                    current_state = ACTIVATED;
                }
                break;
            case ALARMING:
                firstActivate = true;
                firstDeactivate = true;
                firstSnooze = true;

                if (firstAlarm) {
                    ESP_LOGI(TAG, "State: Alarming");
                    alarm_end = add_time(&now, 0, ALARM_LENGTH_MIN, 0);
                    ESP_LOGI(TAG, "Alarm End: ");
                    ds_log_time(&alarm_end);
                    blinking = true;
                    firstAlarm = false;
                }

                if (stop_requested) {
                    stop_requested = false;
                    current_state = ACTIVATED;
                }
                
                if (ds_compare_time(&now, &alarm_end) >= 0) { // Alarm over
                    current_state = DEACTIVATED;
                }

                ls_set_rgb(&alarm_color);
            case SNOOZING:

        }

        vTaskDelay(30);
    }

}