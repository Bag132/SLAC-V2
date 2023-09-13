#include <driver/ledc.h>

typedef struct {
    uint8_t r, g, b;
} ls_color;


void ls_setup(int8_t red_gpio, int8_t green_gpio, int8_t blue_gpio);

void ls_set_rgb(ls_color *color);

void ls_time_fade(ls_color *color, int fade_time);

void ls_step_fade(ls_color *color, uint32_t step, uint32_t interval);

void ls_set_fade_callback(ledc_cbs_t *callback, void *user_arg);
