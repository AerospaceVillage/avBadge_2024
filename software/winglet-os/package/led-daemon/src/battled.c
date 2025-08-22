#include "ledDaemon.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BLINK_RATE 1
#define CHARGING_THRESHOLD_UA 100000

static int battled_show_percentage(unsigned int percentage, int charging) {
    int ret;
    double hue;

    const rgb black = {.r = 0, .g = 0, .b = 0};

    static int blink = 0;
    static int blink_idx = 0;
    blink_idx++;
    if (blink_idx >= BLINK_RATE) {
        blink_idx = 0;
        blink = !blink;
    }

    if (!charging) {
        blink = 0;
        blink_idx = 0;
    }

    if (charging) {
        hue = 220;
    }
    else {
        hue = pow(1.2, 0.291761180811 * percentage) - 1;

        if (hue > 120) {
            hue = 120;
        }
        else if (hue < 0) {
            hue = 0;
        }
    }

    // Compute colors
    hsv hcolor = {.h = hue, .s = (charging ? 0.92 : 1.0), .v = 0.05};
    rgb color = hsv2rgb(hcolor);
    double integrald;
    double last_ratio = modf(percentage / 25.0, &integrald);
    int integral = integrald;
    if (last_ratio == 0) {
        last_ratio = 1;
        integral -= 1;
    }
    rgb last_color = {.r = color.r * last_ratio, .g = color.g * last_ratio, .b = color.b * last_ratio};

    // Set Lower LEDs
    for (int i = LED_BATT_MIN; i < LED_BATT_MIN + integral; i++) {
        ret = rgbled_set(i, color);
        if (ret) return ret;
    }

    // Set actual LED
    if (integral >= 0) {
        ret = rgbled_set(LED_BATT_MIN + integral, blink ? black : last_color);
        if (ret) return ret;
    }
    else {
        // all 0, just set smallest red
        rgb empty_color = {.r = charging ? 0 : 1/255.0, .g = 0, .b = charging ? 1/255.0 : 0};
        ret = rgbled_set(LED_BATT_MIN, blink ? black : empty_color);
        integral = 0;
    }

    // Clear upper LEDs
    for (int i = LED_BATT_MIN + integral + 1; i <= LED_BATT_MAX; i++) {
        ret = rgbled_set(i, black);
        if (ret) return ret;
    }

    return 0;
}

static int battery_get_percentage(void) {
    FILE* file = fopen("/sys/class/power_supply/battery/capacity", "r"); /* should check the result */
    if (!file) {
        return -1;
    }

    char line[32];
    int capacity = -1;

    if (fgets(line, sizeof(line), file)) {
        capacity = atoi(line);
    }

    fclose(file);

    return capacity;
}

static int battery_get_charging(void) {
    FILE* file = fopen("/sys/class/power_supply/battery/current_now", "r"); /* should check the result */
    if (!file) {
        return -1;
    }

    char line[32];
    int current = 0;

    if (fgets(line, sizeof(line), file)) {
        current = atoi(line);
    }

    fclose(file);

    return current > CHARGING_THRESHOLD_UA;
}



static bool show_batt_status = true;
static bool needs_clear = true;

void batt_set_enable(bool enable) {
    show_batt_status = enable;
    batt_update_leds();
}

void batt_update_leds(void) {
    if (show_batt_status) {
        int percentage = battery_get_percentage();
        int charging = battery_get_charging();
        if (percentage < 0)
            perror("batt_get_percentage");
        else if (charging < 0)
            perror("batt_get_charging");
        else {
            battled_show_percentage(percentage, charging);
        }
        needs_clear = 1;
    }
    else {
        if (needs_clear) {
            rgbled_clearallBat();
        }
        needs_clear = false;
    }
}
