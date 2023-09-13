#include "light_strip.h"
#include <math.h>

#define RED_CHANNEL     LEDC_CHANNEL_0
#define GREEN_CHANNEL   LEDC_CHANNEL_1
#define BLUE_CHANNEL    LEDC_CHANNEL_2
#define PWM_EXPONENT 2.32558139535f

const float exponent = 2.32558139535f;


void ls_setup(int8_t red_gpio_num, int8_t green_gpio_num, int8_t blue_gpio_num)
{
    ledc_fade_func_install(0);

    gpio_reset_pin(red_gpio_num);
    gpio_reset_pin(green_gpio_num);
    gpio_reset_pin(blue_gpio_num);

    gpio_set_direction(red_gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_direction(green_gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_direction(blue_gpio_num, GPIO_MODE_OUTPUT);

    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_HIGH_SPEED_MODE, 
        .timer_num = LEDC_TIMER_0, 
        .freq_hz = 400, 
        .duty_resolution = LEDC_TIMER_8_BIT, 
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_cfg = {
        .channel = RED_CHANNEL, 
        .duty = 0, 
        .gpio_num = red_gpio_num, 
        .speed_mode = LEDC_HIGH_SPEED_MODE, 
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };

    ledc_channel_config(&channel_cfg);
    
    channel_cfg.channel = GREEN_CHANNEL;
    channel_cfg.gpio_num = green_gpio_num;
    ledc_channel_config(&channel_cfg);

    channel_cfg.channel = BLUE_CHANNEL;
    channel_cfg.gpio_num = blue_gpio_num;
    ledc_channel_config(&channel_cfg);
}

static float signumf(float x) {
    if (x > 0.f) return 1.f;
    if (x < 0.f) return -1.f;
    return 1.f;
}

// TODO: Use lookup table instead
static float getExponential(float input, float exponent, float weight, float deadband) {
        // printf("input = %f\n", input);

		if (fabs(input) < deadband) {
            printf("input < deadband\n");
			return 0.f;
		}
		float sign = signumf(input);
		float v = fabs(input);
        // printf("v = %f\n", v);

		float a = weight * powf(v, exponent) + (1.f - weight) * v;
		float b = weight * powf(deadband, exponent) + (1.f - weight) * deadband;
		v = (a - 1.f * b) / (1.f - b);

		v *= sign;
		return v;
	}


// TODO: Use color struct
void ls_set_rgb(ls_color *color)
{
    // const float exponent = 4.2f, weight = 1.f;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, RED_CHANNEL, (uint32_t) (getExponential(((float) color->r / 255.f), exponent, 1.f, 0.f) * 255.f));
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL, (uint32_t) (getExponential(((float) color->g / 255.f), exponent, 1.f, 0.f) * 255.f));
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, (uint32_t) (getExponential(((float) color->b / 255.f), exponent, 1.f, 0.f) * 255.f));
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, RED_CHANNEL);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL);
}

void ls_time_fade(ls_color *color, int fade_time)
{
    uint32_t redv = (uint32_t) (getExponential(((float) color->r / 255.f), exponent, 1.f, 0.f) * 255.f);
    uint32_t greenv = (uint32_t) (getExponential(((float) color->g / 255.f), exponent, 1.f, 0.f) * 255.f);
    uint32_t bluev = (uint32_t) (getExponential(((float) color->b / 255.f), exponent, 1.f, 0.f) * 255.f);

    ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, RED_CHANNEL, redv, fade_time);
    ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL, greenv, fade_time);
    ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, bluev, fade_time);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, RED_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, LEDC_FADE_NO_WAIT);
}

void ls_step_fade(ls_color *color, uint32_t step, uint32_t interval)
{
    uint32_t redv = (uint32_t) (getExponential(((float) color->r / 255.f), exponent, 1.f, 0.f) * 255.f);
    uint32_t greenv = (uint32_t) (getExponential(((float) color->g / 255.f), exponent, 1.f, 0.f) * 255.f);
    uint32_t bluev = (uint32_t) (getExponential(((float) color->b / 255.f), exponent, 1.f, 0.f) * 255.f);

    ledc_set_fade_with_step(LEDC_HIGH_SPEED_MODE, RED_CHANNEL, redv, step, interval);
    ledc_set_fade_with_step(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL, greenv, step, interval);
    ledc_set_fade_with_step(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, bluev, step, interval);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, RED_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, LEDC_FADE_NO_WAIT);
}

void ls_set_fade_callback(ledc_cbs_t *callback, void *user_arg)
{
    ledc_cb_register(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, callback, user_arg);
}
