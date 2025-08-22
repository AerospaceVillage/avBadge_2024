#ifndef RGBLEDS_H_
#define RGBLEDS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Common Defs
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

#define LED_COUNT 24
#define LED_RING_COUNT 20
#define LED_BATT_MIN 20
#define LED_BATT_MAX 23
enum led_channel {
    LED_RED = 0,
    LED_GREEN = 1,
    LED_BLUE = 2,

    // Must be last
    LED_CHANNELS
};

// Defined in rgbled.c
hsv rgb2hsv(rgb in);
rgb hsv2rgb(hsv in);
int rgbled_init(void);
int rgbled_clearall(void);
void rgbled_cleanup(void);
int rgbled_set(unsigned int led, rgb color);
int rgbled_clearallRing(void);
int rgbled_clearallBat(void);
int regbled_testbatt(void);

// Defined in battled.c
void batt_set_enable(bool enable);
void batt_update_leds(void);

#ifdef __cplusplus
}
#endif

#endif
