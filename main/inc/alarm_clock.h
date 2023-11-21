#include <DS1307.h>

ds_time get_next_alarm_time(const ds_time *alarm_time);

void alarm_run();

void alarm_set_alarm_time(ds_time t);

void alarm_stop();
