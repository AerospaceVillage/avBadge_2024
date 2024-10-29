#include "appsettings.h"
#include <QMetaProperty>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "winglet-ui/worker/gpsreceiver.h"
#include "appsettingspropentry.h"
#include "timezonesetting.h"
#include "wifienablesetting.h"

#define SETTINGS_PATH "/var/ui-settings.json"
#define GPS_SAVE_INTERVAL_MS 60000

namespace WingletUI {

AppSettings::AppSettings(QObject *parent)
    : QObject{parent}, m_settingsEntryRoot(new SettingsEntryContainer(this, "Settings"))
{
    loadSettings();

    // Define the settings menu
    SettingsEntryContainer* wifiSettings = new SettingsEntryContainer(this, "WiFi");
    wifiEnableSettingPtr = new WifiEnableSetting(this);  // Need to save to pointer so we can notify when wifi monitor is online
    wifiSettings->addEntry(wifiEnableSettingPtr);
    wifiSettings->addEntry(new SettingsActionEntry(this, "Add WiFi Network", ACTION_JOIN_WIFI));
    wifiSettings->addEntry(new SettingsActionEntry(this, "Known Networks", ACTION_MANAGE_WIFI_NETWORKS));
    m_settingsEntryRoot->addEntry(wifiSettings);

    SettingsEntryContainer* ioSettings = new SettingsEntryContainer(this, "Badge I/O");
    ioSettings->addEntry(new AppSettingsBoolPropEntry(this, "externalAntenna", "External Antenna"));
    ioSettings->addEntry(new AppSettingsBoolPropEntry(this, "saoPowerEn", "SAO Power Enable"));
    ioSettings->addEntry(new AppSettingsBoolPropEntry(this, "saoI2CPullEn", "SAO I2C Pull Up En"));
    m_settingsEntryRoot->addEntry(ioSettings);

    SettingsEntryContainer* uiSettings = new SettingsEntryContainer(this, "UI Options");
    uiSettings->addEntry(new AppSettingsEnumPropEntry(this, "screenBrightness", "Brightness", {
                                                                   {250, "25%"},
                                                                   {500, "50%"},
                                                                   {750, "75%"},
                                                                   {1000, "100%"}
                                                               }));
    uiSettings->addEntry(new AppSettingsEnumPropEntry(this, "adsbDecayTimeSec", "ADSB Timeout", {
                                                                   {15, "15 Seconds"},
                                                                   {30, "30 Seconds"},
                                                                   {60, "1 Minute"},
                                                                   {120, "2 Minutes"},
                                                                   {180, "3 Minutes"},
                                                                   {300, "5 Minutes"},
                                                               }));
    uiSettings->addEntry(new TimeZoneSetting(this));
    uiSettings->addEntry(new AppSettingsBoolPropEntry(this, "invertedScrollDirection", "Invert Scrolling"));
    uiSettings->addEntry(new AppSettingsDoublePropEntry(this, "lastLatitude","Cached Latitude",
                                                              "(Only kept till next GPS Lock)\nEnter Latitude:",
                                                               {}, -90, 90));
    uiSettings->addEntry(new AppSettingsDoublePropEntry(this, "lastLongitude","Cached Longitude",
                                                               "(Only kept till next GPS Lock)\nEnter Longitude:",
                                                               {}, -180, 180));
    m_settingsEntryRoot->addEntry(uiSettings);

    m_settingsEntryRoot->addEntry(new SettingsActionEntry(this, "About Device", ACTION_ABOUT));
    m_settingsEntryRoot->addEntry(new SettingsActionEntry(this, "Join Private Discord", ACTION_PRIVATE_DISCORD));
}

void AppSettings::loadSettings()
{
    if (!QFile::exists(SETTINGS_PATH)) {
        // Can't load settings if they don't exist
        return;
    }

    QFile inFile(SETTINGS_PATH);
    if (!inFile.open(QFile::ReadOnly)) {
        // Couldn't open file, just fail silently (saving will report any I/O problems)
        return;
    }

    QJsonParseError error;
    auto settingsDoc = QJsonDocument::fromJson(inFile.readAll(), &error);
    if (settingsDoc.isNull()) {
        qWarning("Failed to decode settings JSON: %s", error.errorString().toLatin1().data());
        return;
    }

    if (!settingsDoc.isObject()) {
        qWarning("Settings root isn't json object");
        return;
    }

    auto settingsObj = settingsDoc.object();
    foreach (const QString& key, settingsObj.keys()) {
        QJsonValue value = settingsObj.value(key);
        const char *keyStr = key.toLatin1().data();
        if (!setProperty(keyStr, value.toVariant())) {
            qWarning("Failed to load setting '%s'", keyStr);
        }
    }
}

void AppSettings::saveSettings()
{
    QJsonObject settingsObj;

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); i++) {
        auto prop = metaObject()->property(i);
        auto val = prop.read(this);

        switch (val.type()) {
        case QVariant::Bool:
            settingsObj.insert(prop.name(), val.toBool());
            break;
        case QVariant::Int:
            settingsObj.insert(prop.name(), val.toInt());
            break;
        case QVariant::UInt:
            settingsObj.insert(prop.name(), (qint64) val.toUInt());
            break;
        case QVariant::Double:
            settingsObj.insert(prop.name(), val.toDouble());
            break;
        case QVariant::String:
            settingsObj.insert(prop.name(), val.toString());
            break;
        default:
            qWarning("Cannot serialize setting '%s': Unknown type %s", prop.name(), prop.typeName());
            break;;
        }
    }

    QJsonDocument settingsDoc(settingsObj);

    QFile outFile(SETTINGS_PATH);
    if (outFile.open(QFile::ReadWrite | QFile::Truncate)) {
        outFile.write(settingsDoc.toJson(QJsonDocument::Compact).data());
        outFile.close();
    }
    else {
        qWarning("Failed to save settings to '%s'", SETTINGS_PATH);
    }
}

void AppSettings::reportGPSReading(GPSReading *reading)
{
    if (!reading->valid)
        return;

    static bool firstSave = true;

    float latDifference = abs(reading->latitude - m_lastLatitude);
    float longDifference = abs(reading->longitude - m_lastLongitude);

    m_lastLatitude = reading->latitude;
    m_lastLongitude = reading->longitude;

    // See if we need to save settings
    bool needsSave = false;
    if (firstSave) {
        needsSave = true;
        firstSave = false;
    }
    else if (latDifference > 0.01 || longDifference > 0.01) {
        needsSave = true;
    }
    else if (gpsSaveTimer.elapsed() > GPS_SAVE_INTERVAL_MS) {
        needsSave = true;
    }

    // Save settings if any of the flags above were true
    if (needsSave) {
        emit lastLatitudeChanged(m_lastLatitude);
        emit lastLongitudeChanged(m_lastLongitude);
        saveSettings();
        gpsSaveTimer.start();
    }
}

void AppSettings::reportWifiMonReady() {
    wifiEnableSettingPtr->reportMonitorReady();
}

} // namespace WingletUI
