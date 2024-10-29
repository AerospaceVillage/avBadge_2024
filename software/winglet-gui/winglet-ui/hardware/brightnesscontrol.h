#ifndef WINGLETUI_BRIGHTNESSCONTROL_H
#define WINGLETUI_BRIGHTNESSCONTROL_H

#include <QObject>

namespace WingletUI {

class BrightnessControl : public QObject
{
    Q_OBJECT
public:
    explicit BrightnessControl(QObject *parent);
    ~BrightnessControl();

private slots:
    void brightnessChanged(int value);

private:
    int brightnessFd = -1;
};

} // namespace WingletUI

#endif // WINGLETUI_BRIGHTNESSCONTROL_H
