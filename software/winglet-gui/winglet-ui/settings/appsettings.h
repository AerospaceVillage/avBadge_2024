#ifndef WINGLETUI_APPSETTINGS_H
#define WINGLETUI_APPSETTINGS_H

#include <QObject>
#include <QElapsedTimer>
#include "abstractsettingsentry.h"

namespace WingletUI {

class GPSReading;

#define DEFINE_SETTING(type, name, defaultVal, setName) \
    public: \
        Q_PROPERTY(type name READ name WRITE setName NOTIFY name ## Changed STORED true) \
        type name() const { return m_ ## name; } \
        void setName(type val) { \
            if (val != m_ ## name) { \
                m_ ## name = val; \
                emit name ## Changed(val); \
                saveSettings(); \
            } \
        } \
    private: \
        type m_ ## name = defaultVal;

class WifiEnableSetting;

class AppSettings : public QObject
{
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr);
    void loadSettings();
    void saveSettings();
    const SettingsEntryContainer *settingsEntryRoot() const { return m_settingsEntryRoot; }
    bool rebootNeeded = false;

    DEFINE_SETTING(bool, externalAntenna, false, setExternalAntenna)
    DEFINE_SETTING(bool, saoPowerEn, true, setSaoPowerEn)
    DEFINE_SETTING(bool, saoI2CPullEn, true, setSaoI2CPullEn)
    DEFINE_SETTING(int, screenBrightness, 750, setScreenBrightness)
    DEFINE_SETTING(bool, clockShowSeconds, true, setClockShowSeconds)
    DEFINE_SETTING(int, adsbDecayTimeSec, 60, setAdsbDecayTimeSec)
    DEFINE_SETTING(bool, invertedScrollDirection, true, setInvertedScrollDirection)
    DEFINE_SETTING(bool, wifiStartDisabled, false, setWifiStartDisabled)

    // Set default latitude/longitude to las vegas convention center
    DEFINE_SETTING(double, lastLatitude, 36.1345, setLastLatitude)
    DEFINE_SETTING(double, lastLongitude, -115.1580, setLastLongitude)

public:
    void reportGPSReading(GPSReading* reading);

    void reportWifiMonReady();  // Needed since when settings initialized, the wifi monitor is still off

    // List of actions that the settings screen can run
    enum AppSettingsAction {
        ACTION_ABOUT,
        ACTION_PRIVATE_DISCORD,
        ACTION_JOIN_WIFI,
        ACTION_MANAGE_WIFI_NETWORKS
    };

signals:
    void externalAntennaChanged(bool val);
    void saoPowerEnChanged(bool val);
    void saoI2CPullEnChanged(bool val);
    void screenBrightnessChanged(int val);
    void clockShowSecondsChanged(bool val);
    void adsbDecayTimeSecChanged(int val);
    void invertedScrollDirectionChanged(bool val);
    void wifiStartDisabledChanged(bool val);
    void lastLatitudeChanged(double val);
    void lastLongitudeChanged(double val);

private:
    QElapsedTimer gpsSaveTimer;
    SettingsEntryContainer *m_settingsEntryRoot;
    WifiEnableSetting *wifiEnableSettingPtr;
};

} // namespace WingletUI

#endif // WINGLETUI_APPSETTINGS_H
