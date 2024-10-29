#ifndef RGBLEDS_H_
#define RGBLEDS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

int rgbled_init(void);
void rgbled_cleanup(void);
int rgbled_set(unsigned int led, rgb color);

#define LED_COUNT 20

enum led_channel {
    LED_RED = 0,
    LED_GREEN = 1,
    LED_BLUE = 2,

    // Must be last
    LED_CHANNELS
};


int rgbled_set(unsigned int led, rgb color);
int rgbled_set_angle(double angle, rgb on_color, rgb off_color);
void rgbled_clear();
int rgbled_show_radar_beam(double angle);
int rgbled_show_color_wheel(double angle);
int rgbled_show_shimmer(double angle);
int rgbled_show_compass(double angle);

#ifdef __cplusplus
}
#endif

#endif
