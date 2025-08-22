#include "gpiocontrol.h"

#ifndef NO_HARDWARE
#include <gpiod.h>
#endif

#include "wingletgui.h"

namespace WingletUI {

#define	CONSUMER	"Winglet GUI"

#define GPIO_LINE(port, pin) (((port) * 32) + (pin))
#define PORTB(pin) GPIO_LINE(1, pin)
#define PORTC(pin) GPIO_LINE(2, pin)
#define PORTD(pin) GPIO_LINE(3, pin)
#define PORTE(pin) GPIO_LINE(4, pin)
#define PORTF(pin) GPIO_LINE(5, pin)
#define PORTG(pin) GPIO_LINE(6, pin)

#define LINE_EXT_ANT PORTG(0)
#define LINE_SAO_POWER_EN PORTG(4)
#define LINE_SAO_I2C_PULL_EN PORTG(5)

#ifdef NO_HARDWARE

GPIOControl::GPIOControl(QObject *parent): QObject{parent} {}
GPIOControl::~GPIOControl() {}
void GPIOControl::externalAntennaChanged(bool extAnt) { (void) extAnt; }
void GPIOControl::saoPowerEnChanged(bool powerEn) { (void) powerEn; }
void GPIOControl::saoI2CPullEnChanged(bool pullEn) { (void) pullEn; }

#else

GPIOControl::GPIOControl(QObject *parent)
    : QObject{parent}
{
    settings = &dynamic_cast<WingletGUI*>(parent)->settings;

    gpioChip = gpiod_chip_open_by_name("gpiochip0");
    if (!gpioChip) {
        qWarning("Failed to open gpio chip: %d", errno);
        return;
    }

    extAntLine = gpiod_chip_get_line(gpioChip, LINE_EXT_ANT);
    if (extAntLine) {
        if (!gpiod_line_request_output(extAntLine, CONSUMER, settings->externalAntenna())) {
            connect(settings, SIGNAL(externalAntennaChanged(bool)), this, SLOT(externalAntennaChanged(bool)));
        }
        else {
            qWarning("Failed to acquire external antenna line: %d", errno);
        }
    }
    else {
        qWarning("Failed to get external antenna line: %d", errno);
    }

    saoPowerEnLine = gpiod_chip_get_line(gpioChip, LINE_SAO_POWER_EN);
    if (saoPowerEnLine) {
        if (!gpiod_line_request_output(saoPowerEnLine, CONSUMER, settings->saoPowerEn())) {
            connect(settings, SIGNAL(saoPowerEnChanged(bool)), this, SLOT(saoPowerEnChanged(bool)));
        }
        else {
            qWarning("Failed to acquire SAO Power Enable line: %d", errno);
        }
    }
    else {
        qWarning("Failed to get SAO Power Enable line: %d", errno);
    }

    saoI2CPullEnLine = gpiod_chip_get_line(gpioChip, LINE_SAO_I2C_PULL_EN);
    if (saoI2CPullEnLine) {
        bool saoI2CPullLvl = !settings->saoI2CPullEn() || !settings->saoPowerEn();
        if (!gpiod_line_request_output(saoI2CPullEnLine, CONSUMER, saoI2CPullLvl)) {
            connect(settings, SIGNAL(saoI2CPullEnChanged(bool)), this, SLOT(saoI2CPullEnChanged(bool)));
        }
        else {
            qWarning("Failed to acquire SAO I2C Pull Enable line: %d", errno);
        }
    }
    else {
        qWarning("Failed to get SAO I2C Pull Enable line: %d", errno);
    }
}

GPIOControl::~GPIOControl()
{
    if (extAntLine)
        gpiod_line_release(extAntLine);
    if (saoPowerEnLine)
        gpiod_line_release(saoPowerEnLine);
    if (saoI2CPullEnLine)
        gpiod_line_release(saoI2CPullEnLine);

    if (gpioChip)
        gpiod_chip_close(gpioChip);
}

void GPIOControl::externalAntennaChanged(bool extAnt) {
    if (!extAntLine)
        return;

    if (gpiod_line_set_value(extAntLine, extAnt)) {
        qWarning("Failed to set external antenna: %d", errno);
    }
}

void GPIOControl::saoPowerEnChanged(bool powerEn) {
    if (!saoPowerEnLine)
        return;

    if (gpiod_line_set_value(saoPowerEnLine, powerEn)) {
        qWarning("Failed to set SAO Power Enable: %d", errno);
    }

    if (!saoI2CPullEnLine)
        return;

    // Same as for i2c pull changed, can only have pull when power is enabled
    bool saoI2CPullLvl = !settings->saoI2CPullEn() || !powerEn;

    if (gpiod_line_set_value(saoI2CPullEnLine, saoI2CPullLvl)) {
        qWarning("Failed to set SAO I2C Pull Enable: %d", errno);
    }
}

void GPIOControl::saoI2CPullEnChanged(bool pullEn) {
    if (!saoI2CPullEnLine || !saoPowerEnLine)
        return;

    // Note pull up enable is inverted from the value (false enables pull ups)
    // SAO Pull Enable can only work if both SAO power enable is true and I2C pull is true (or else we'll get leakage)
    bool saoI2CPullLvl = !pullEn || !settings->saoPowerEn();

    if (gpiod_line_set_value(saoI2CPullEnLine, saoI2CPullLvl)) {
        qWarning("Failed to set SAO I2C Pull Enable: %d", errno);
    }
}

#endif

} // namespace WingletUI
