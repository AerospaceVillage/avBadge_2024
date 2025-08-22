#ifndef WINGLETUI_LEDCONTROL_H
#define WINGLETUI_LEDCONTROL_H

#include <QObject>

namespace WingletUI {
// Ring LED Numbers
//
//               10          11
//                @@@@@@@@@@@@@      12
//         9  @@@@            @@@@
//           @@@                  @@@    13
//      8  @@                       @@@
//        @@                          @@   14
//    7  @@                            @@
//      @@                              @@   15
//  6  @@                                @@
//     @@                                @@
//     @@                                @@
//  5  @@                                @@   16
//      @@                              @@
//    4  @@                            @@   17
//        @@                         @@@
//     3   @@@                      @@   18
//           @@@                  @@@
//         2    @@@@          @@@@   19
//                  @@@@@@@@@@
//                1           0
//

class LedControl : public QObject
{
    Q_OBJECT
public:
    const static int LED_COUNT = 20;

    explicit LedControl(QObject *parent, const char* fifoPath = "/var/run/ledFiFo");
    ~LedControl();

    void clearRing();
    void clearRingLed(uint led);
    void setRingLed(uint led, uint r, uint g, uint b);
    static double ledToAngle(uint led);

    enum ledTheme { 
        no_led_theme,
        gps_led_theme
    };
    void setLedTheme(ledTheme theme);
    void stepLedTheme(int themeValue);

private slots:
    void ledBrightnessChanged(int val);
    void battLedEnableChanged(bool val);
private:
    void setBrightnessLevel(int level);
    void setBattLedEnable(bool enable);
    void sendFifoCmd(const QString& cmd);
    
    // Theme stepping.
    ledTheme currentTheme = no_led_theme;
    void stepGpsTheme(int themeValue);

    int fifoFd;
};

} // namespace WingletUI

#endif // WINGLETUI_BRIGHTNESSCONTROL_H
