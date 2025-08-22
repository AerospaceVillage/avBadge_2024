#include "ledDaemon.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#define LED_PATH_MAX_LEN 41
#define LED_PATH_FMT "/sys/class/leds/sunxi_led%d%c/brightness"

static const char chan_lookup[LED_CHANNELS] = {'r', 'g', 'b'};
//static const double gamma_fix[LED_CHANNELS] = {1.6, 2.8, 2.0};
static const double gamma_fix[LED_CHANNELS] = {1.0, 1.0, 1.0};
static int led_fds[LED_COUNT][LED_CHANNELS] = {};

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = 0.0;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}

rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;
}

static int rgbled_set_chan_internal(unsigned int led, enum led_channel chan, double brightness_float) {
    int fd = led_fds[led][chan];
    if (fd <= 0) {
        errno = EBADFD;
        return 1;
    }

    // Perform gamma correction

    int brightness;
    if (brightness_float < 0) {
        brightness = 0;
    }
    else if (brightness_float > 1) {
        brightness = 255;
    }
    else {
        brightness = (int)(pow(brightness_float, gamma_fix[chan]) * 255);
    }

    char brightness_str[5];

    int len = snprintf(brightness_str, sizeof(brightness_str), "%d\n", brightness);
    if ((len < 0) || ((unsigned int) len) >= sizeof(brightness_str)) {
        errno = EOVERFLOW;
        return 1;
    }

    int ret = write(fd, brightness_str, len);
    if (ret != len) {
        return 1;
    }

    // Give time for RGB strip to update
    usleep(100);

    return 0;
}

int rgbled_set(unsigned int led, rgb color) {

    if (led >= LED_COUNT) {
        return 0;
    }

    int ret;

    ret = rgbled_set_chan_internal(led, LED_RED, color.r);
    if (ret) return ret;

    ret = rgbled_set_chan_internal(led, LED_GREEN, color.g);
    if (ret) return ret;

    ret = rgbled_set_chan_internal(led, LED_BLUE, color.b);
    if (ret) return ret;


    return 0;
}

/// @brief  clear all the leds
/// @return
int rgbled_clearall() {
    int led, ret;

    for (led = 0; led < LED_COUNT; led++) {

        ret = rgbled_set_chan_internal(led, LED_RED, 0);
        if (ret) return ret;

        ret = rgbled_set_chan_internal(led, LED_GREEN, 0);
        if (ret) return ret;

        ret = rgbled_set_chan_internal(led, LED_BLUE, 0);
        if (ret) return ret;
    }
    return 0;
}

/// @brief clear all the Ring leds
/// @return
int rgbled_clearallRing() {
    int led, ret;

    for (led = 0; led < LED_RING_COUNT; led++) {

        ret = rgbled_set_chan_internal(led, LED_RED, 0);
        if (ret) return ret;

        ret = rgbled_set_chan_internal(led, LED_GREEN, 0);
        if (ret) return ret;

        ret = rgbled_set_chan_internal(led, LED_BLUE, 0);
        if (ret) return ret;
    }
    return 0;
}

/// @brief clear all battery leds
/// @return
int rgbled_clearallBat() {
    int led, ret;

    for (led = LED_BATT_MIN; led <LED_BATT_MAX + 1; led++) {
        ret = rgbled_set_chan_internal(led, LED_RED, 0);
        if (ret) return ret;

        ret = rgbled_set_chan_internal(led, LED_GREEN, 0);
        if (ret) return ret;

        ret = rgbled_set_chan_internal(led, LED_BLUE, 0);
        if (ret) return ret;
    }
    return 0;
}

void rgbled_cleanup() {
    int led, chan;

    for (led = 0; led < LED_COUNT; led++) {
        for (chan = 0; chan < LED_CHANNELS; chan++) {
            if (led_fds[led][chan] > 0) {
                rgbled_set_chan_internal(led, (enum led_channel)chan, 0);
                close(led_fds[led][chan]);
                led_fds[led][chan] = 0;
            }
        }
    }
}

// Inizialize LED
int rgbled_init(void) {
    char led_path[LED_PATH_MAX_LEN];
    int led, chan, ret;

    for (led = 0; led < LED_COUNT; led++) {
        for (chan = 0; chan < LED_CHANNELS; chan++) {
            ret = snprintf(led_path, sizeof(led_path), LED_PATH_FMT, led, chan_lookup[chan]);
            if (ret >= sizeof(led_path)) {
                ret = EOVERFLOW;
                goto cleanup;
            }

            ret = open(led_path, O_WRONLY);
            if (ret <= 0) {
                ret = errno;
                goto cleanup;
            }
            led_fds[led][chan] = ret;
        }
    }

    return 0;

cleanup:
    rgbled_cleanup();
    errno = ret;
    return -1;
}

int regbled_testbatt(void) {
    for (int i = LED_BATT_MIN; i < LED_BATT_MAX + 1; i++) {
        rgb color = {.r = 0.05, .g = 0.05, .b = 0.05};
        int ret = rgbled_set(i, color);
        if (ret) return ret;
    }

    return 0;
}
