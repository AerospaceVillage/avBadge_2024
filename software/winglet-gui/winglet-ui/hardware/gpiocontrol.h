#ifndef WINGLETUI_GPIOCONTROL_H
#define WINGLETUI_GPIOCONTROL_H

#include <QObject>

struct gpiod_chip;
struct gpiod_line;

namespace WingletUI {

class AppSettings;

class GPIOControl : public QObject
{
    Q_OBJECT
public:
    explicit GPIOControl(QObject *parent);
    ~GPIOControl();

private slots:
    void externalAntennaChanged(bool extAnt);
    void saoPowerEnChanged(bool powerEn);
    void saoI2CPullEnChanged(bool pullEn);

private:
    struct gpiod_chip *gpioChip = NULL;
    struct gpiod_line *extAntLine = NULL;
    struct gpiod_line *saoPowerEnLine = NULL;
    struct gpiod_line *saoI2CPullEnLine = NULL;
    AppSettings *settings;
};

} // namespace WingletUI

#endif // WINGLETUI_GPIOCONTROL_H
