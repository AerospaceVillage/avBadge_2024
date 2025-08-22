#ifndef WINGLETUI_APPSETTINGS_H
#define WINGLETUI_APPSETTINGS_H

#include <QObject>
#include <QElapsedTimer>
#include "abstractsettingsentry.h"

#define USB_ROLE_HOST 1
#define USB_ROLE_DEVICE 2
#define USB_ROLE_AUTO 3

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
        Q_SIGNAL void name ## Changed(type newVal); \
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
    DEFINE_SETTING(int, ledBrightness,9, setLedBrightness)
    DEFINE_SETTING(bool, battLedEnable, true, setBattLedEnable)
    DEFINE_SETTING(bool, darkMode, true, setDarkMode)
    DEFINE_SETTING(bool, timeFormat12hr, false, setTimeFormat12hr)
    DEFINE_SETTING(bool, sdCardMaps, false, setSdCardMaps)
    DEFINE_SETTING(int, usbRole, USB_ROLE_AUTO, setUsbRole)

    // Keeps fast charge 3A setting in persistent location to survive reboots
    DEFINE_SETTING(bool, fastCharge3APersistent, false, setFastCharge3APersistent)

    // Set default latitude/longitude to las vegas convention center
    DEFINE_SETTING(double, lastLatitude, 36.1345, setLastLatitude)
    DEFINE_SETTING(double, lastLongitude, -115.1580, setLastLongitude)

    // Holds defaults for resuming canard control settings
    DEFINE_SETTING(uint, canardLastFmFreq, 1017, setCanardLastFmFreq)
    DEFINE_SETTING(uint, canardLastFmPreset, 1, setCanardLastFmPreset)
    DEFINE_SETTING(uint, canardLastAirbandFreq, 121500, setCanardLastAirbandFreq)
    DEFINE_SETTING(uint, canardLastAirbandPreset, 1, setCanardLastAirbandPreset)

public:
    void reportGPSReading(GPSReading* reading);

    void reportWifiMonReady();  // Needed since when settings initialized, the wifi monitor is still off

    // List of actions that the settings screen can run
    enum AppSettingsAction {
        ACTION_ABOUT,
        ACTION_PRIVATE_DISCORD,
        ACTION_WIFI_SCAN,
        ACTION_WIFI_MANUAL,
        ACTION_MANAGE_WIFI_NETWORKS,
        ACTION_CLEAR_ROOT_PASSWORD,
        ACTION_RELEASE_NOTES,
        ACTION_COLD_BOOT_GPS
    };

private:
    QElapsedTimer gpsSaveTimer;
    SettingsEntryContainer *m_settingsEntryRoot;
    WifiEnableSetting *wifiEnableSettingPtr;
};

} // namespace WingletUI

#endif // WINGLETUI_APPSETTINGS_H
