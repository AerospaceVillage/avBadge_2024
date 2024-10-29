#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "rgbleds.h"

#define LED_PATH_FMT "/sys/class/leds/sunxi_led%d%c/brightness"
#define LED_PATH_MAX_LEN 41
static const char chan_lookup[LED_CHANNELS] = {'r', 'g', 'b'};
static const double gamma_fix[LED_CHANNELS] = {1.6, 2.8, 2.0};
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
        out.h = NAN;                            // its now undefined
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

int rgbled_init() {
    char led_path[LED_PATH_MAX_LEN];
    int led, chan, ret;

    for (led = 0; led < LED_COUNT; led++) {
        for (chan = 0; chan < LED_CHANNELS; chan++) {
            ret = snprintf(led_path, sizeof(led_path), LED_PATH_FMT, led, chan_lookup[chan]);
            if (ret < 0 || ((unsigned int)ret) >= sizeof(led_path)) {
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

void rgbled_clear(){
    for(int i = 0; i<LED_COUNT; i++){
        rgb color = {};
        rgbled_set(i, color);
    }
}


// ========================================
// Begin Demo Code
// ========================================

static inline double led_to_angle(int led) {
    // LED 0 is the LED directly right from the bottom post of the screen
    // LED 1 goes clockwise from that position
    // We are treating up as 0 degrees

    // Pitch between LEDs in radians
    const double led_pitch = 2.0 * M_PI / 20.0;

    // All LEDs are all shifted by half an led distance from north (mounting pegs are aligned)
    // And we are the 9th LED counting clockwise from North
    const double led0_angle = led_pitch * 9.5;

    return led0_angle + (led * led_pitch);
}

int rgbled_show_radar_beam(double angle) {
    // Angle is in radians between 0 and 2*pi, with 0 pointing up from the screen, and the
    // angle increasing clockwise

    for (int i = 0; i < 20; i++) {
        rgb color = {};
        color.g = 3 * cos(led_to_angle(i) - angle) - 2.0;

        int ret = rgbled_set(i, color);
        if (ret) return ret;
    }

    return 0;
}
int rgbled_show_compass(double angle) {
    // Angle is in radians between 0 and 2*pi, with 0 pointing up from the screen, and the
    // angle increasing clockwise

    for (int i = 0; i < 20; i++) {
        rgb color = {};
        color.r = 1;
        if(led_to_angle(i)== angle){
            int ret = rgbled_set(i, color);
            if (ret) return ret;
        }
    }

    return 0;
}

int rgbled_show_color_wheel(double angle) {
    for (int i = 0; i < 20; i++) {
        double this_angle = led_to_angle(i) + angle;

        double integral;
        hsv incolor;
        incolor.h = modf(this_angle / (2 * M_PI), &integral) * 360;
        incolor.s = 1;
        incolor.v = 0.6;
        int ret = rgbled_set(i, hsv2rgb(incolor));
        if (ret) return ret;
    }

    return 0;
}

int rgbled_show_shimmer(double angle) {
    // Angle is in radians between 0 and 2*pi, with 0 pointing up from the screen, and the
    // angle increasing clockwise

    for (int i = 0; i < 20; i++) {
        rgb color = {};
        color.r = 0.7 * (0.40 * cos((led_to_angle(i) - sin(3*angle)) * 4) + 0.60);
        color.g = 0.7 * (0.1 * cos((led_to_angle(i) - sin(3*angle)) * 4) + 0.9);
        color.b = 0.7;

        int ret = rgbled_set(i, color);
        if (ret) return ret;
    }

    return 0;
}


// int main() {
//     int ret = rgbled_init();
//     if (ret) {
//         perror("rgbled_init");
//         return 1;
//     }

//     for (int idx = 0; idx < 400; idx++) {
//         ret = rgbled_show_color_wheel((399 - idx) / 100.0 * 2.0*M_PI);
//         if (ret) goto cleanup;
//         usleep(5000);
//     }

//     for (int idx = 0; idx < 400; idx++) {
//         ret = rgbled_show_radar_beam(idx / 50.0 * 2.0*M_PI);
//         if (ret) goto cleanup;
//         usleep(5000);
//     }

//     for (int idx = 0; idx < 400; idx++) {
//         ret = rgbled_show_shimmer((399 - idx) / 100.0 * 2.0*M_PI);
//         if (ret) goto cleanup;
//         usleep(5000);
//     }

//     // rgbled_set_chan_internal(23, LED_BLUE, 64);

//     // ret = rgbled_show_shimmer(0.5 * M_PI);
//     // usleep(1000000);



// cleanup:
//     if (ret) perror("rgbled_set");
//     rgbled_cleanup();
//     return 0;
// }
