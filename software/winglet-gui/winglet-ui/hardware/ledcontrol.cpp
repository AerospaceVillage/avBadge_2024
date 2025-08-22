#include "ledcontrol.h"
#include "wingletgui.h"
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

namespace WingletUI {

LedControl::LedControl(QObject *parent, const char* fifoPath)
    : QObject{parent}
{
    // Connect to the FIFO
    fifoFd = open(fifoPath, O_WRONLY | O_NONBLOCK);
    if (fifoFd < 0) {
        qWarning("LedControl::LedControl(): Failed to connect to LED daemon: %d", errno);
    }

    // Register to receive settings updates
    AppSettings *settings = &dynamic_cast<WingletGUI*>(parent)->settings;
    connect(settings, SIGNAL(ledBrightnessChanged(int)), this, SLOT(ledBrightnessChanged(int)));
    connect(settings, SIGNAL(battLedEnableChanged(bool)), this, SLOT(battLedEnableChanged(bool)));
    setBrightnessLevel(settings->ledBrightness());
    setBattLedEnable(settings->battLedEnable());
}

LedControl::~LedControl()
{
    if (fifoFd >= 0)
        close(fifoFd);
}

// ========================================
// Utility Functions
// ========================================

double LedControl::ledToAngle(uint led) {
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

void LedControl::sendFifoCmd(const QString& cmd) {
    if (fifoFd < 0)
        return;

    QByteArray cmdEncoded = cmd.toLatin1();
    int ret = write(fifoFd, cmdEncoded.constData(), cmdEncoded.size() + 1);  // Can do this since data is null terminated when we convert from string
    if (ret < 0) {
        int err = errno;
        qWarning("LedControl::sendFifoCmd: Failed to send command '%s' (%d)", cmdEncoded.constData(), err);
    }
    else if (ret == 0) {
        qWarning("LedControl::sendFifoCmd: Failed to send command '%s' (fifo closed)", cmdEncoded.constData());
    }
    else if (ret != cmd.size() + 1) {
        qWarning("LedControl::sendFifoCmd: Failed to send command '%s' (only sent %d bytes)", cmdEncoded.constData(), ret);
    }
}

// ========================================
// Settings Slots
// ========================================

void LedControl::ledBrightnessChanged(int val){
    setBrightnessLevel(val);
}

void LedControl::battLedEnableChanged(bool val) {
    setBattLedEnable(val);
}

// ========================================
// Command Implementations
// ========================================

/**
 * @brief Sets the brightness level of an LED.
 *
 * This function takes an integer representing the desired brightness level,
 * performs input validation, adjusts the value if it's below a threshold,
 * formats a command string, writes the command to a file descriptor,
 * and pauses execution briefly.
 *
 * @param brig The brightness level to set (0-9).
 * @return 0 on success, -1 on error (invalid brightness level).
 */
void LedControl::setBrightnessLevel(int level) {
    if (level < 0 || level > 9) {
        qWarning("LedControl::clearRingLed: Attempting to set invalid brightness %d", level);
        return;
    }

    QString cmd = QStringLiteral("LEDBRIG%1").arg(level);
    sendFifoCmd(cmd);
}

/**
 * @brief Enables/Disables the bottom battery LEDs on the badge
 * @param enable Set to true to enable, false to clear
 */
void LedControl::setBattLedEnable(bool enable) {
    sendFifoCmd(enable ? "LEDBATEN1" : "LEDBATEN0");
}
/**
 * Clear all leds in the Ring.
 */
void LedControl::clearRing() {
    sendFifoCmd("CLEARALLR");
}

/**
 * Clears the specified LED in the Ring.
 *
 * @param led The index of the LED to clear (0-20).
 *
 * @return 0 on success, -1 on error.
 */
void LedControl::clearRingLed(uint led) {
    if (led >= LED_COUNT) {
        qWarning("LedControl::clearRingLed: Attempting to clear invalid led %d", led);
        return;
    }
    QString cmd = QStringLiteral("LED%1RGB000").arg(led, 2, 10, QLatin1Char('0'));
    sendFifoCmd(cmd);
}

/**
 * Sets the color of the specified LED in the Ring.
 *
 * @param led The index of the LED to set (0-20).
 * @param r The red component of the color (0-9).
 * @param g The green component of the color (0-9).
 * @param b The blue component of the color (0-9).
 *
 * @return 0 on success, -1 on error.
 */
void LedControl::setRingLed(uint led, uint r, uint g, uint b) {
    if (led >= LED_COUNT) {
        qWarning("LedControl::setRingLed: Attempting to set invalid led %d", led);
        return;
    }
    if (r > 255 || g > 255 || b > 255) {
        qWarning("LedControl::setRingLed: Attempting to set invalid led value %d,%d,%d", r, g, b);
        return;
    }
    const QLatin1Char ZERO('0');
    QString cmd = QStringLiteral("LED%1RGB%2%3%4").arg(led, 2, 10, ZERO)
            .arg(r, 2, 16, ZERO)
            .arg(g, 2, 16, ZERO)
            .arg(b, 2, 16, ZERO);
    sendFifoCmd(cmd);
}

void LedControl::setLedTheme(ledTheme theme) {
    if (currentTheme == theme) {
        return;
    }
    // Reset LEDs on theme change.
    clearRing();
    
    switch(theme) {
        case no_led_theme: currentTheme=theme; break;
        case gps_led_theme: currentTheme=theme; break;
    }
}

void LedControl::stepGpsTheme(int themeValue) {
    const static int gpsLedSequence[LED_COUNT] = {10, 11, 9, 12, 8, 13, 7, 14, 6, 15, 5, 16, 4, 17, 3, 18, 2, 19, 1, 0};
    // TODO: Cornercase analysis, off by ones, etc.
    if (themeValue > LED_COUNT) {
        themeValue = LED_COUNT;
    }

    for (int i=0; i<LED_COUNT; i++) {
        if (i<themeValue) {
            setRingLed(gpsLedSequence[i], 0, 255, 0);
        }
        else {
            setRingLed(gpsLedSequence[i], 0, 0, 0);
        }
    }

}

void LedControl::stepLedTheme(int themeValue) {
    switch(this->currentTheme) {
        case no_led_theme: break;
        case gps_led_theme: stepGpsTheme(themeValue); break;
    }
}


} // namespace WingletUI
