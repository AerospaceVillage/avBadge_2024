#ifndef WINGLETUI_WIFIENABLESETTING_H
#define WINGLETUI_WIFIENABLESETTING_H

#include <QObject>
#include "abstractsettingsentry.h"

namespace WingletUI {

class WifiEnableSetting : public WingletUI::AbstractBoolSetting
{
    Q_OBJECT
public:
    explicit WifiEnableSetting(QObject *parent);

    void reportMonitorReady();

    bool value() const override;
    void setValue(bool val) override;

private slots:
    void wifiStateChanged(int state, int wifiStrength);

private:
    bool monitorReady = false;
    bool cachedWifiOnVal = false;
};

} // namespace WingletUI

#endif // WINGLETUI_WIFIENABLESETTING_H
