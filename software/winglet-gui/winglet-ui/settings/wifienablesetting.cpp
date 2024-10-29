#include "wifienablesetting.h"

#include <QTimer>
#include "wingletgui.h"

namespace WingletUI {

WifiEnableSetting::WifiEnableSetting(QObject *parent)
    : WingletUI::AbstractBoolSetting(parent, "WiFi Enable") {}

void WifiEnableSetting::reportMonitorReady() {
    if (monitorReady)
        return;

    monitorReady = true;

    connect(WingletGUI::inst->wifiMon, SIGNAL(wifiStateChanged(int, int)), this, SLOT(wifiStateChanged(int, int)));
    cachedWifiOnVal = WingletGUI::inst->wifiMon->wifiState() != WifiMonitor::WIFI_OFF;
    reportChanged();

    // Try lowering the interface on startup if we are requested to start disabled
    if (WingletGUI::inst->settings.wifiStartDisabled()) {
        WingletGUI::inst->wifiMon->lowerInterface();
    }
}

void WifiEnableSetting::wifiStateChanged(int state, int wifiStrength) {
    (void) wifiStrength;

    bool wifiIsOn = state != WifiMonitor::WIFI_OFF;
    bool lastWifiWasOn = cachedWifiOnVal;
    cachedWifiOnVal = wifiIsOn;

    if (wifiIsOn != lastWifiWasOn) {
        reportChanged();
    }
}

bool WifiEnableSetting::value() const {
    if (!monitorReady)
        return cachedWifiOnVal;

    return WingletGUI::inst->wifiMon->wifiState() != WifiMonitor::WIFI_OFF;
}

void WifiEnableSetting::setValue(bool value) {
    if (!monitorReady)
        return;

    if (value) {
        WingletGUI::inst->wifiMon->raiseInterface();
        WingletGUI::inst->settings.setWifiStartDisabled(false);
    }
    else {
        WingletGUI::inst->wifiMon->lowerInterface();
        WingletGUI::inst->settings.setWifiStartDisabled(true);
    }
}

} // namespace WingletUI
