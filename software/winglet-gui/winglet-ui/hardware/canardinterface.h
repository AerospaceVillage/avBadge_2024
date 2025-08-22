#ifndef CANARDINTERFACE_H
#define CANARDINTERFACE_H

#include <QObject>
#include "sao/canard_constants.h"

namespace WingletUI {

class SAOMonitor;
class CanardSettings;

class CanardInterface: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectionStateChanged)
public:
    struct CanardFreq{
        short wholeFreq;
        short decFreq;
    };

    CanardInterface(SAOMonitor *parent = nullptr);
    ~CanardInterface();

    // Connection Management
    bool connected() { return m_connected; }
signals:
    void connectionStateChanged(bool connected);

    // Private Signals to trigger canard settings events across threads
    void triggerLoadSettings(QPrivateSignal);
    void triggerInvalidateSettings(QPrivateSignal);

    // --- Old API REMOVE ME ---
public:
    CanardFreq getFreq();
    bool setFreq(CanardFreq);

    // ====================
    // New API
    // ====================

    // Use this to query presets & preset counts
    CanardSettings* settings;

    // Number of decimal places of precision for tuning freq on
    static const uint FM_DECIMAL_COUNT = CANARD_RADIO_FM_DECIMAL_COUNT;
    static const uint AIRBAND_DECIMAL_COUNT = CANARD_RADIO_AIRBAND_DECIMAL_COUNT;

    enum RadioMode {
        MODE_FM = 0,
        MODE_AIRBAND = 1,
    };

    struct RadioState {
        RadioMode mode;
        bool presetMode;
        union {
            uint preset;     // Used if preset mode. Starts at 1, up to PRESET_COUNT defined in settings
            uint32_t freq;   // Used if ! preset mode - see *_DECIMAL_COUNT for scale factor
        } tuning;

        uint getDecimalCount() const {
            int numDecimal = 0;
            switch (mode) {
            case CanardInterface::MODE_FM:
                numDecimal = FM_DECIMAL_COUNT;
                break;
            case CanardInterface::MODE_AIRBAND:
                numDecimal = AIRBAND_DECIMAL_COUNT;
                break;
            }
            return numDecimal;
        }

        uint32_t getMaxFreq() const {
            switch (mode) {
            case CanardInterface::MODE_FM:
                return CANARD_RADIO_FM_MAX_FREQ;
            case CanardInterface::MODE_AIRBAND:
                return CANARD_RADIO_AIRBAND_MAX_FREQ;
            default:
                return 0;
            }
        }

        uint32_t getMinFreq() const {
            switch (mode) {
            case CanardInterface::MODE_FM:
                return CANARD_RADIO_FM_MIN_FREQ;
            case CanardInterface::MODE_AIRBAND:
                return CANARD_RADIO_AIRBAND_MIN_FREQ;
            default:
                return 0;
            }
        }
    };

    RadioState getRadioState() { return m_cachedRadioState; }
    void setRadioState(const RadioState& newState);
    uint8_t getVolume() { return m_cachedVolume; }
    void setVolume(uint8_t volume);

    // Set this to enable/disable badge control
    bool setBadgeControlMode(bool saoUnderBadgeCtrl);


protected:
    friend class CanardSettings;
    uint8_t address = 0xFF;
    bool loadSettingsFromDevice(uint8_t* settingsOut);
    bool saveSettingsToDevice(const uint8_t* settingsOut);

    friend class SAOMonitor;
    void commt_setConnected(bool connected);
    void commt_ping();

private:
    bool m_connected = false;

    // Cached state for access by UI thread
    SAOMonitor *m_mon = nullptr;
    RadioState m_cachedRadioState = {};
    uint8_t m_cachedVolume = 0;
    uint8_t m_cachedSettings[CANARD_SETTINGS_LEN] = {};

private slots:
    // Set this to enable/disable badge control
    bool commt_setBadgeControl(bool saoUnderBadgeCtrl);

    // Canard Communication Primitives
    void commt_setRadioState(QByteArray state);
    void commt_setVolume(uint volume);
    bool commt_writeSettings(QByteArray settings);
    bool commt_setCtrlEnable(bool ctrlEnable);
    bool commt_readoutRadioState();
    bool commt_readoutVolume();
    bool commt_readoutSettings(bool ignoreNoConnected = false);
};


} // namespace WingletUI

#endif // CANARDINTERFACE_H
