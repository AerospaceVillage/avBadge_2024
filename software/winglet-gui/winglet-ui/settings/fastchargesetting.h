#ifndef WINGLETUI_FASTCHARGESETTING_H
#define WINGLETUI_FASTCHARGESETTING_H

#include "abstractsettingsentry.h"

namespace WingletUI {

class AppSettings;

class FastChargeSetting : public AbstractBoolSetting
{
    Q_OBJECT
public:
    explicit FastChargeSetting(AppSettings *parent = nullptr);

    int value() const override;
    void setValue(bool val) override;

private:
    AppSettings *settings;
    static bool readRegVal(uint8_t* regOut);
};

} // namespace WingletUI

#endif // WINGLETUI_FASTCHARGESETTING_H
