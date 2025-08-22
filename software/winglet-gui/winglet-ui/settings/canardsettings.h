#ifndef WINGLETUI_CANARDSETTINGS_H
#define WINGLETUI_CANARDSETTINGS_H

#include <QObject>
#include <QValidator>
#include <QMetaProperty>
#include "winglet-ui/hardware/sao/canard_constants.h"
#include "winglet-ui/windowcore/circularkeyboard.h"
#include "abstractsettingsentry.h"

namespace WingletUI {

// Forward declare (we need to include this file from CanardInterface so we'll do this)
class CanardInterface;

// ========================================
// Settings Rendering Classes
// ========================================

class FrequencyValidator : public QValidator
{
    Q_OBJECT
public:
    explicit FrequencyValidator(uint32_t minFreq, uint32_t maxFreq, uint32_t freqDec, QObject *parent = nullptr):
        QValidator{parent}, m_minFreq(minFreq), m_maxFreq(maxFreq), m_freqDec(freqDec) {}

    State validate ( QString& input, int& pos ) const override;

    bool convertTextToFreq(const QString& input, uint32_t &freqOut) const;
    static QString convertFreqToTextStatic(uint32_t freq, uint32_t freqDec);
    QString convertFreqToText(uint32_t freq) const { return convertFreqToTextStatic(freq, m_freqDec); }

private:
    uint32_t m_minFreq;
    uint32_t m_maxFreq;
    uint32_t m_freqDec;
};

class CanardSettingsFreqEntry : public AbstractTextSetting
{
    Q_OBJECT
public:
    CanardSettingsFreqEntry(QObject *propOwner, const char * propName, QString displayName,
                               uint32_t minFreq, uint32_t maxFreq, uint32_t freqDec, QPixmap icon = {});
    // freqDiv is the division factor from the uint32_t frequency to decimal frequency

    QString value() const override;
    QString displayValue() const override;
    void setValue(const QString &val) override;
    QList<QString> inputKeys() const override { return kbdChars; }
    QString prompt() const override;
    int maxLength() const override { return 8; } // Allow 4 digits before, 3 digits after, 1 decimal, validator will catch the rest
    QString title() const override;
    QValidator* validator() const override;
    bool allowEmptyInput() const override;
    QString validatorFailedMsg() const override;

protected slots:
    void settingUpdated(uint newVal);

private:
    QList<QString> kbdChars = {"0123456789."};
    QObject *propOwner;
    uint32_t m_minFreq;
    uint32_t m_maxFreq;
    uint32_t m_freqDec;
    FrequencyValidator *m_validator;
    QMetaProperty prop;
};

class CanardSettingsPresetNameEntry : public AbstractTextSetting
{
    Q_OBJECT
public:
    CanardSettingsPresetNameEntry(QObject *propOwner, const char * propName, QString displayName, QPixmap icon = {});

    QString value() const override;
    void setValue(const QString &val) override;
    QList<QString> inputKeys() const override { return CircularKeyboard::fullKeyboard; }
    QString prompt() const override;
    int maxLength() const override { return 12; } // Preset names are 12 characters
    QString title() const override;
    bool allowEmptyInput() const override { return false; };

protected slots:
    void settingUpdated(QString newVal);

private:
    QObject *propOwner;
    QMetaProperty prop;
};


// ========================================
// Settings Type Macros
// ========================================

#define DEFINE_SETTING_BOOL(offset, name, setName) \
    Q_PROPERTY(bool name READ name WRITE setName NOTIFY name ## Changed STORED false) \
    bool name() const { return !!m_settingsArr[offset]; } \
    void setName(bool val) { \
        if (val != name()) { \
            m_settingsArr[offset] = val; \
            m_settingsChanged = true; \
            emit name ## Changed(val); \
        } \
    } \
    Q_SIGNAL void name ## Changed(bool newVal);

#define DEFINE_SETTING_U8(offset, name, setName) \
    Q_PROPERTY(uint name READ name WRITE setName NOTIFY name ## Changed STORED false) \
    uint name() const { return m_settingsArr[offset]; } \
    void setName(uint val) { \
        if (val != name()) { \
            m_settingsArr[offset] = val; \
            m_settingsChanged = true; \
            emit name ## Changed(val); \
        } \
    } \
    Q_SIGNAL void name ## Changed(uint newVal);

#define DEFINE_SETTING_ENUM_U8(offset, name, setName) \
    Q_PROPERTY(int name READ name WRITE setName NOTIFY name ## Changed STORED false) \
    int name() const { return m_settingsArr[offset]; } \
    void setName(int val) { \
        if (val != name()) { \
            m_settingsArr[offset] = val; \
            m_settingsChanged = true; \
            emit name ## Changed(val); \
        } \
    } \
    Q_SIGNAL void name ## Changed(int newVal);

#define DEFINE_SETTING_U16(offset, name, setName) \
    Q_PROPERTY(uint name READ name WRITE setName NOTIFY name ## Changed STORED false) \
    uint name() const { return m_settingsArr[offset] | ((uint16_t)m_settingsArr[offset + 1] << 8); } \
    void setName(uint val) { \
        if (val != name()) { \
            m_settingsArr[offset] = val & 0xFF; \
            m_settingsArr[offset + 1] = val >> 8; \
            m_settingsChanged = true; \
            emit name ## Changed(val); \
        } \
    } \
    Q_SIGNAL void name ## Changed(uint newVal);

#define DEFINE_SETTING_U32(offset, name, setName) \
    Q_PROPERTY(uint name READ name WRITE setName NOTIFY name ## Changed STORED false) \
    uint name() const { \
        return m_settingsArr[offset] | ((uint32_t)m_settingsArr[offset + 1] << 8) | \
            ((uint32_t)m_settingsArr[offset + 2] << 16) | ((uint32_t)m_settingsArr[offset + 3] << 24); \
    } \
    void setName(uint val) { \
        if (val != name()) { \
            m_settingsArr[offset] = val & 0xFF; \
            m_settingsArr[offset + 1] = val >> 8; \
            m_settingsArr[offset + 2] = val >> 16; \
            m_settingsArr[offset + 3] = val >> 24; \
            m_settingsChanged = true; \
            emit name ## Changed(val); \
        } \
    } \
    Q_SIGNAL void name ## Changed(uint newVal);

#define DEFINE_SETTING_FIXLENSTR(offset, size, name, setName) \
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY name ## Changed STORED false) \
    QString name() const { \
        return getFromFixedStr(offset, size); \
    } \
    void setName(QString val) { \
        if (val != name()) { \
            writeToFixedStr(offset, size, val); \
            m_settingsChanged = true; \
            emit name ## Changed(val); \
        } \
    } \
    Q_SIGNAL void name ## Changed(QString newVal);


// ========================================
// Canard Settings Definitions
// ========================================

class CanardSettings: public QObject
{
    Q_OBJECT

public:
    CanardSettings(QObject *parent = nullptr);

    // ========================================
    // For Settings Control
    // ========================================

    const SettingsEntryContainer *settingsEntryRoot() const { return m_settingsEntryRoot; }
    bool needsSave() const { return m_settingsChanged; }
    bool settingsValid() const { return m_settingsValid; }

    bool loadSettings();
    bool saveSettings();


    // ========================================
    // Utility Functions
    // ========================================

    // Get Presets
    static const uint FM_PRESET_COUNT = 10;
    static const uint AIRBAND_PRESET_COUNT = 10;

    QString getFmPresetName(uint presetNum);  // NOTE: Presets start at 1
    QString getFmPresetFreq(uint presetNum);
    QString getAirbandPresetName(uint presetNum);
    QString getAirbandPresetFreq(uint presetNum);


    // ========================================
    // Settings Definitions
    // ========================================

    // Checksum at start
    static const uint SETTINGS_ARR_SIZE = CANARD_SETTINGS_LEN;
    static const uint CRC16_LSB_OFFSET = 0;
    static const uint CRC16_MSB_OFFSET = 1;
    static const uint CRC_CALC_START_OFFSET = 2;

    static const uint VERSION_MAJOR_EXPECTED = 1;
    static const uint VERSION_MAJOR_OFFSET = 2;
    static const uint VERSION_MINOR_EXPECTED = 0;
    static const uint VERSION_MINOR_OFFSET = 3;

    // Start of real settings
    DEFINE_SETTING_BOOL(4, firstPartyEqEn, setFirstPartyEqEn)
    DEFINE_SETTING_ENUM_U8(5, sleepTimeout, setSleepTimeout)
    DEFINE_SETTING_BOOL(6, useExternalAnt, setUseExternalAnt)

    // FM Presets
    DEFINE_SETTING_U16(16, fmPreset1, setFmPreset1)
    DEFINE_SETTING_U16(18, fmPreset2, setFmPreset2)
    DEFINE_SETTING_U16(20, fmPreset3, setFmPreset3)
    DEFINE_SETTING_U16(22, fmPreset4, setFmPreset4)
    DEFINE_SETTING_U16(24, fmPreset5, setFmPreset5)
    DEFINE_SETTING_U16(26, fmPreset6, setFmPreset6)
    DEFINE_SETTING_U16(28, fmPreset7, setFmPreset7)
    DEFINE_SETTING_U16(30, fmPreset8, setFmPreset8)
    DEFINE_SETTING_U16(32, fmPreset9, setFmPreset9)
    DEFINE_SETTING_U16(34, fmPreset10, setFmPreset10)

    // Airband Presets
    DEFINE_SETTING_U32(36, airbandPreset1, setAirbandPreset1)
    DEFINE_SETTING_U32(40, airbandPreset2, setAirbandPreset2)
    DEFINE_SETTING_U32(44, airbandPreset3, setAirbandPreset3)
    DEFINE_SETTING_U32(48, airbandPreset4, setAirbandPreset4)
    DEFINE_SETTING_U32(52, airbandPreset5, setAirbandPreset5)
    DEFINE_SETTING_U32(56, airbandPreset6, setAirbandPreset6)
    DEFINE_SETTING_U32(60, airbandPreset7, setAirbandPreset7)
    DEFINE_SETTING_U32(64, airbandPreset8, setAirbandPreset8)
    DEFINE_SETTING_U32(68, airbandPreset9, setAirbandPreset9)
    DEFINE_SETTING_U32(72, airbandPreset10, setAirbandPreset10)

    // FM Preset Names
    DEFINE_SETTING_FIXLENSTR(76, 12, fmPreset1Name, setFmPreset1Name)
    DEFINE_SETTING_FIXLENSTR(88, 12, fmPreset2Name, setFmPreset2Name)
    DEFINE_SETTING_FIXLENSTR(100, 12, fmPreset3Name, setFmPreset3Name)
    DEFINE_SETTING_FIXLENSTR(112, 12, fmPreset4Name, setFmPreset4Name)
    DEFINE_SETTING_FIXLENSTR(124, 12, fmPreset5Name, setFmPreset5Name)
    DEFINE_SETTING_FIXLENSTR(136, 12, fmPreset6Name, setFmPreset6Name)
    DEFINE_SETTING_FIXLENSTR(148, 12, fmPreset7Name, setFmPreset7Name)
    DEFINE_SETTING_FIXLENSTR(160, 12, fmPreset8Name, setFmPreset8Name)
    DEFINE_SETTING_FIXLENSTR(172, 12, fmPreset9Name, setFmPreset9Name)
    DEFINE_SETTING_FIXLENSTR(184, 12, fmPreset10Name, setFmPreset10Name)

    // Airband Preset Names
    DEFINE_SETTING_FIXLENSTR(196, 12, airbandPreset1Name, setAirbandPreset1Name)
    DEFINE_SETTING_FIXLENSTR(208, 12, airbandPreset2Name, setAirbandPreset2Name)
    DEFINE_SETTING_FIXLENSTR(220, 12, airbandPreset3Name, setAirbandPreset3Name)
    DEFINE_SETTING_FIXLENSTR(232, 12, airbandPreset4Name, setAirbandPreset4Name)
    DEFINE_SETTING_FIXLENSTR(244, 12, airbandPreset5Name, setAirbandPreset5Name)
    DEFINE_SETTING_FIXLENSTR(256, 12, airbandPreset6Name, setAirbandPreset6Name)
    DEFINE_SETTING_FIXLENSTR(268, 12, airbandPreset7Name, setAirbandPreset7Name)
    DEFINE_SETTING_FIXLENSTR(280, 12, airbandPreset8Name, setAirbandPreset8Name)
    DEFINE_SETTING_FIXLENSTR(292, 12, airbandPreset9Name, setAirbandPreset9Name)
    DEFINE_SETTING_FIXLENSTR(304, 12, airbandPreset10Name, setAirbandPreset10Name)


    // ========================================
    // Signals from Interface Thread
    // ========================================

protected slots:
    friend class CanardInterface;
    void loadSettingsFromMon();
    void invalidateSettingsFromMon();
    void setInterface(CanardInterface* interface) { m_interface = interface; }


    // ========================================
    // Private State
    // ========================================

private:
    CanardInterface *m_interface = nullptr;
    uint8_t m_settingsArr[SETTINGS_ARR_SIZE] = {};
    bool m_settingsValid = false;
    bool m_settingsChanged = false;
    SettingsEntryContainer *m_settingsEntryRoot;

    QString getFromFixedStr(uint index, uint fixedLen) const;
    void writeToFixedStr(uint index, uint fixedLen, const QString& msg);

    void addFmPresetEntry(SettingsEntryContainer* container, int idx);
    void addAirbandPresetEntry(SettingsEntryContainer* container, int idx);
    QVariant lookupProperty(QString propName);
};

} // namespace WingletUI

#endif // WINGLETUI_CANARDSETTINGS_H
