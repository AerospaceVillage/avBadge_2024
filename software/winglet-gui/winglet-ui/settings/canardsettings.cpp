#include "canardsettings.h"
#include "appsettingspropentry.h"
#include "winglet-ui/hardware/canardinterface.h"

namespace WingletUI {

// ========================================
// Frequency Validation/Conversion
// ========================================

bool FrequencyValidator::convertTextToFreq(const QString &input, uint32_t &freqOut) const {
    if (input.isEmpty())
        return false;

    // Make sure decScale won't overflow
    Q_ASSERT(m_freqDec < 10);
    uint32_t decScale = 1;
    for (uint32_t i = 0; i < m_freqDec; i++) {
        decScale *= 10;
    }
    uint32_t maxInt = UINT32_MAX / decScale;

    // Get integer part
    uint32_t freqInt = 0;
    int i;
    for (i = 0; i < input.length(); i++) {
        char c = input[i].toLatin1();
        // Decimal point, break early
        if (c == '.')
            break;
        // Invalid character
        if (c < '0' || c > '9')
            return false;

        // Bounds check before adding on
        if (freqInt > UINT32_MAX / 10) {
            return false;
        }
        else if (freqInt == UINT32_MAX / 10 && freqInt > UINT32_MAX % 10) {
            return false;
        }

        // Add on next part
        freqInt *= 10;
        freqInt += (c - '0');
    }

    // Integer component of frequency is too large, won't fit
    if (freqInt > maxInt) {
        return false;
    }
    freqInt *= decScale;

    if (i < input.length()) {
        // We've got a fractional part, process it
        i++;
        // Check for trailing decimal point (illegal)
        if (i >= input.length())
            return false;

        // Process remaining part of string
        uint32_t remainingScale = decScale / 10;
        for (; i < input.length(); i++) {
            char c = input[i].toLatin1();
            // Invalid character
            if (c < '0' || c > '9')
                return false;

            // Only allow 0s after precision
            if (remainingScale == 0 && c != '0') {
                return false;
            }

            // Add on next part
            uint32_t value = (c - '0') * remainingScale;
            freqInt += value;
            remainingScale /= 10;
        }
    }

    freqOut = freqInt;
    return true;
}

QString FrequencyValidator::convertFreqToTextStatic(uint32_t freq, uint32_t freqDec) {
    char data[12];  // We can fit at most 10 digits and a decimal point in a uint32
    data[11] = 0;
    int i = 11;
    int decRem = freqDec;
    while (freq != 0) {
        data[--i] = '0' + (freq % 10);
        freq /= 10;
        decRem--;
        if (decRem == 0) {
            data[--i] = '.';
            if (!freq)
                data[--i] = '0';
        }
    }

    if (decRem > 0) {
        while (decRem) {
            data[--i] = '0';
            decRem--;
        }
        data[--i] = '.';
        data[--i] = '0';
    }

    return QString(&data[i]);
}

FrequencyValidator::State FrequencyValidator::validate(QString& input, int& pos) const {
    (void) pos;

    if (input.isEmpty())
        return Invalid;

    uint32_t val;
    if (!convertTextToFreq(input, val))
        return Invalid;

    if (val >= m_minFreq && val <= m_maxFreq) {
        return Acceptable;
    }
    else {
        return Invalid;
    }
}

// ========================================
// Canard Frequency Property Entry
// ========================================

CanardSettingsFreqEntry::CanardSettingsFreqEntry(QObject *propOwner, const char * propName, QString displayName,
                                                 uint32_t minFreq, uint32_t maxFreq, uint32_t freqDec, QPixmap icon):
    AbstractTextSetting(propOwner, displayName, icon), propOwner(propOwner), m_minFreq(minFreq),
    m_maxFreq(maxFreq), m_freqDec(freqDec)
{
    const QMetaObject* meta = propOwner->metaObject();
    int propIdx = meta->indexOfProperty(propName);
    if (propIdx < 0) {
        qWarning("CanardSettingsFreqEntry: Failed to locate property '%s' in class '%s'", propName, meta->className());
        return;
    }

    QMetaProperty propInfo = meta->property(propIdx);
    if (!propInfo.isReadable() || !propInfo.isWritable()) {
        qWarning("CanardSettingsFreqEntry: Cannot initialize using property '%s' from class '%s': Not readable/writable", propName, meta->className());
        return;
    }
    if (propInfo.type() != QVariant::UInt) {
        qWarning("CanardSettingsFreqEntry: Cannot initialize using property '%s' from class '%s': Not a uint", propName, meta->className());
        return;
    }
    prop = propInfo;

    m_validator = new FrequencyValidator(m_minFreq, m_maxFreq, m_freqDec, this);

    // Connect the settings updated signal the property
    QMetaObject::connect(propOwner, propInfo.notifySignalIndex(), this, metaObject()->indexOfSlot("settingUpdated(uint)"));
}

QString CanardSettingsFreqEntry::value() const
{
    if (!prop.isValid())
        return "0.0";

    QVariant val = prop.read(propOwner);
    return m_validator->convertFreqToText(val.toUInt());
}

QString CanardSettingsFreqEntry::displayValue() const
{
    return value() + " MHz";
}

void CanardSettingsFreqEntry::setValue(const QString &val)
{
    if (!prop.isValid())
        return;

    uint32_t freqOut;
    if (!m_validator->convertTextToFreq(val, freqOut)) {
        return;
    }
    if (freqOut < m_minFreq || freqOut > m_maxFreq) {
        return;
    }

    prop.write(propOwner, (uint) freqOut);
}

void CanardSettingsFreqEntry::settingUpdated(uint newVal) {
    (void) newVal;
    reportChanged();
}

QString CanardSettingsFreqEntry::prompt() const
{
    return QStringLiteral("Set Frequency (MHz):");
}

QString CanardSettingsFreqEntry::title() const
{
    return name();
}

QValidator* CanardSettingsFreqEntry::validator() const
{
    return m_validator;
}

bool CanardSettingsFreqEntry::allowEmptyInput() const
{
    return false;
}

QString CanardSettingsFreqEntry::validatorFailedMsg() const
{
    return QString("Must be frequency between %1 and %2\n(Up to %3 decimal places)")
            .arg(m_validator->convertFreqToText(m_minFreq))
            .arg(m_validator->convertFreqToText(m_maxFreq))
            .arg(m_freqDec);
}

class CanardSettingsFMFreqEntry: public CanardSettingsFreqEntry {
public:
    CanardSettingsFMFreqEntry(QObject *propOwner, const char * propName, QString displayName):
        CanardSettingsFreqEntry(propOwner, propName, displayName, CANARD_RADIO_FM_MIN_FREQ, CANARD_RADIO_FM_MAX_FREQ, CANARD_RADIO_FM_DECIMAL_COUNT) {}
};

class CanardSettingsAirbandFreqEntry: public CanardSettingsFreqEntry {
public:
    CanardSettingsAirbandFreqEntry(QObject *propOwner, const char * propName, QString displayName):
        CanardSettingsFreqEntry(propOwner, propName, displayName, CANARD_RADIO_AIRBAND_MIN_FREQ, CANARD_RADIO_AIRBAND_MAX_FREQ, CANARD_RADIO_AIRBAND_DECIMAL_COUNT) {}
};

// ========================================
// Canard Fixed Text Input
// ========================================

CanardSettingsPresetNameEntry::CanardSettingsPresetNameEntry(QObject *propOwner, const char * propName, QString displayName,
                                                             QPixmap icon):
    AbstractTextSetting(propOwner, displayName, icon), propOwner(propOwner)
{
    const QMetaObject* meta = propOwner->metaObject();
    int propIdx = meta->indexOfProperty(propName);
    if (propIdx < 0) {
        qWarning("CanardSettingsPresetNameEntry: Failed to locate property '%s' in class '%s'", propName, meta->className());
        return;
    }

    QMetaProperty propInfo = meta->property(propIdx);
    if (!propInfo.isReadable() || !propInfo.isWritable()) {
        qWarning("CanardSettingsPresetNameEntry: Cannot initialize using property '%s' from class '%s': Not readable/writable", propName, meta->className());
        return;
    }
    if (propInfo.type() != QVariant::String) {
        qWarning("CanardSettingsPresetNameEntry: Cannot initialize using property '%s' from class '%s': Not a QString", propName, meta->className());
        return;
    }
    prop = propInfo;

    // Connect the settings updated signal the property
    QMetaObject::connect(propOwner, propInfo.notifySignalIndex(), this, metaObject()->indexOfSlot("settingUpdated(QString)"));
}

QString CanardSettingsPresetNameEntry::value() const
{
    if (!prop.isValid())
        return "0.0";

    QVariant val = prop.read(propOwner);
    return val.toString();
}

void CanardSettingsPresetNameEntry::setValue(const QString &val)
{
    if (!prop.isValid())
        return;

    prop.write(propOwner, val);
}

void CanardSettingsPresetNameEntry::settingUpdated(QString newVal) {
    (void) newVal;
    reportChanged();
}

QString CanardSettingsPresetNameEntry::prompt() const
{
    return QStringLiteral("Set Preset Name:");
}

QString CanardSettingsPresetNameEntry::title() const
{
    return name();
}

// ========================================
// Canard Settings
// ========================================

CanardSettings::CanardSettings(QObject *parent)
    : QObject{parent}, m_settingsEntryRoot(new SettingsEntryContainer(this, "Tuner Settings"))
{
    SettingsEntryContainer* fmPresets = new SettingsEntryContainer(this, "FM Presets");
    for (uint i = 1; i <= FM_PRESET_COUNT; i++) {
        addFmPresetEntry(fmPresets, i);
    }
    m_settingsEntryRoot->addEntry(fmPresets);

    SettingsEntryContainer* airbandPresets = new SettingsEntryContainer(this, "Airband Presets");
    for (uint i = 1; i <= AIRBAND_PRESET_COUNT; i++) {
        addAirbandPresetEntry(airbandPresets, i);
    }
    m_settingsEntryRoot->addEntry(airbandPresets);

    m_settingsEntryRoot->addEntry(new AppSettingsBoolPropEntry(this, "firstPartyEqEn", "Included HP EQ"));
    m_settingsEntryRoot->addEntry(new AppSettingsBoolPropEntry(this, "useExternalAnt", "Ext Antenna"));
    m_settingsEntryRoot->addEntry(new AppSettingsEnumPropEntry(this, "sleepTimeout", "Sleep Timeout", {
                                                                   {0, "Never"},
                                                                   {1, "1 Minute"},
                                                                   {2, "2 Minutes"},
                                                                   {5, "5 Minutes"}
                                                               }));
}

void CanardSettings::addFmPresetEntry(SettingsEntryContainer* container, int idx) {
    char freqPropName[11] = "fmPreset##";
    char namePropName[15] = "fmPreset##Name";
    int nameStartIdx;
    if (idx < 10) {
        freqPropName[8] = idx + '0';
        freqPropName[9] = 0;
        namePropName[8] = idx + '0';
        nameStartIdx = 9;
    }
    else {
        Q_ASSERT(idx < 100);
        freqPropName[8] = (idx / 10) + '0';
        freqPropName[9] = (idx % 10) + '0';
        freqPropName[10] = 0;
        namePropName[8] = (idx / 10) + '0';
        namePropName[9] = (idx % 10) + '0';
        nameStartIdx = 10;
    }
    namePropName[nameStartIdx++] = 'N';
    namePropName[nameStartIdx++] = 'a';
    namePropName[nameStartIdx++] = 'm';
    namePropName[nameStartIdx++] = 'e';
    namePropName[nameStartIdx] = 0;

    QString presetIdxName = QStringLiteral("Preset %1").arg(idx);

    container->addEntry(new CanardSettingsPresetNameEntry(this, namePropName, presetIdxName + " Name"));
    container->addEntry(new CanardSettingsFMFreqEntry(this, freqPropName, presetIdxName));
}

void CanardSettings::addAirbandPresetEntry(SettingsEntryContainer* container, int idx) {
    char freqPropName[16] = "airbandPreset##";
    char namePropName[20] = "airbandPreset##Name";
    int nameStartIdx;
    if (idx < 10) {
        freqPropName[13] = idx + '0';
        freqPropName[14] = 0;
        namePropName[13] = idx + '0';
        nameStartIdx = 14;
    }
    else {
        Q_ASSERT(idx < 100);
        freqPropName[13] = (idx / 10) + '0';
        freqPropName[14] = (idx % 10) + '0';
        freqPropName[15] = 0;
        namePropName[13] = (idx / 10) + '0';
        namePropName[14] = (idx % 10) + '0';
        nameStartIdx = 15;
    }
    namePropName[nameStartIdx++] = 'N';
    namePropName[nameStartIdx++] = 'a';
    namePropName[nameStartIdx++] = 'm';
    namePropName[nameStartIdx++] = 'e';
    namePropName[nameStartIdx] = 0;

    QString presetIdxName = QStringLiteral("Preset %1").arg(idx);

    container->addEntry(new CanardSettingsPresetNameEntry(this, namePropName, presetIdxName + " Name"));
    container->addEntry(new CanardSettingsAirbandFreqEntry(this, freqPropName, presetIdxName));
}

QString CanardSettings::getFromFixedStr(uint index, uint fixedLen) const {
    char data[fixedLen+1];
    uint i;
    for (i = 0; i < fixedLen; i++) {
        if (!m_settingsArr[index+i])
            break;
        data[i] = m_settingsArr[index+i];
    }
    data[i] = 0;
    return QString(data);
}

void CanardSettings::writeToFixedStr(uint index, uint fixedLen, const QString& msg) {
    uint i;
    for (i = 0; (int) i < msg.length() && i < fixedLen; i++) {
        m_settingsArr[index+i] = msg[i].toLatin1();
    }
    // Pad remainder with 0s
    while (i < fixedLen) {
        m_settingsArr[index+i] = 0;
        i++;
    }
}

static const QString unknownStr = QStringLiteral("???");

QVariant CanardSettings::lookupProperty(QString propName) {
    int propIdx = metaObject()->indexOfProperty(propName.toLatin1().constData());
    if (propIdx < 0) {
        return {};
    }
    QMetaProperty propInfo = metaObject()->property(propIdx);
    return propInfo.read(this);
}

QString CanardSettings::getFmPresetName(uint presetNum) {
    QVariant val = lookupProperty(QString("fmPreset%1Name").arg(presetNum));
    if (!val.isValid()) {
        return unknownStr;
    }
    return val.toString();
}
QString CanardSettings::getFmPresetFreq(uint presetNum) {
    QVariant val = lookupProperty(QString("fmPreset%1").arg(presetNum));
    if (!val.isValid()) {
        return unknownStr;
    }
    return FrequencyValidator::convertFreqToTextStatic(val.toUInt(), 1) + " MHz";
}
QString CanardSettings::getAirbandPresetName(uint presetNum) {
    QVariant val = lookupProperty(QString("airbandPreset%1Name").arg(presetNum));
    if (!val.isValid()) {
        return unknownStr;
    }
    return val.toString();
}
QString CanardSettings::getAirbandPresetFreq(uint presetNum) {
    QVariant val = lookupProperty(QString("airbandPreset%1").arg(presetNum));
    if (!val.isValid()) {
        return unknownStr;
    }
    return FrequencyValidator::convertFreqToTextStatic(val.toUInt(), 3) + " MHz";
}

bool CanardSettings::loadSettings() {
    m_settingsValid = false;

    if (!m_interface->loadSettingsFromDevice(m_settingsArr)) {
        return false;
    }

    uint16_t calc_checksum = qChecksum((char*) (m_settingsArr + CRC_CALC_START_OFFSET), SETTINGS_ARR_SIZE - CRC_CALC_START_OFFSET, Qt::ChecksumIso3309);
    uint16_t expected_checksum = m_settingsArr[CRC16_LSB_OFFSET] | ((uint16_t)m_settingsArr[CRC16_MSB_OFFSET] << 8);
    if (calc_checksum != expected_checksum) {
        qWarning("CanardSettings::loadSettings(): Invalid Canard Settings Checksum: Expected 0x%04X but calculated 0x%04X", expected_checksum, calc_checksum);
        return false;
    }
    if (m_settingsArr[VERSION_MAJOR_OFFSET] != VERSION_MAJOR_EXPECTED) {
        qWarning("CanardSettings::loadSettings(): Invalid Canard Settings: Settings are version %d.%d, (this only accepts major version %d)", m_settingsArr[VERSION_MAJOR_OFFSET],
                 m_settingsArr[VERSION_MINOR_OFFSET], VERSION_MAJOR_EXPECTED);
        return false;
    }
    if (m_settingsArr[VERSION_MINOR_OFFSET] < VERSION_MINOR_EXPECTED) {
        qWarning("CanardSettings::loadSettings(): Invalid Canard Settings: Settings are version %d.%d, (this requires a minimum minor version of %d)", m_settingsArr[VERSION_MAJOR_OFFSET],
                 m_settingsArr[VERSION_MINOR_OFFSET], VERSION_MINOR_EXPECTED);
        return false;
    }

    // Iterate over all QProperties (this class is a base qobject so the only properties are settings)
    // Siganl that all of them have changed since we just loaded a new settings array
    const QMetaObject* metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); i++) {
        QMetaProperty objProperty(metaObj->property(i));
        QVariant val = objProperty.read(this);
        if (val.isValid()) {
            QGenericArgument arg = QGenericArgument(val.typeName(), val.data());
            if (!objProperty.notifySignal().invoke(this, arg)) {
                qWarning("CanardSettings::loadSettings(): Failed to notify property '%s'", objProperty.name());
            }
        }
        else {
            qWarning("CanardSettings::loadSettings(): Failed to notify property '%s' update", objProperty.name());
        }
    }

    m_settingsChanged = false;
    m_settingsValid = true;
    return true;
}

bool CanardSettings::saveSettings() {
    if (!m_settingsValid)
        return false;

    uint16_t calc_checksum = qChecksum((char*) (m_settingsArr + CRC_CALC_START_OFFSET), SETTINGS_ARR_SIZE - CRC_CALC_START_OFFSET, Qt::ChecksumIso3309);
    m_settingsArr[CRC16_LSB_OFFSET] = calc_checksum & 0xFF;
    m_settingsArr[CRC16_MSB_OFFSET] = calc_checksum >> 8;

    if (!m_interface->saveSettingsToDevice(m_settingsArr)) {
        // Failed to save, reload settings from device
        loadSettings();
        return false;
    }

    m_settingsChanged = false;
    return true;
}

void CanardSettings::loadSettingsFromMon() {
    loadSettings();
}

void CanardSettings::invalidateSettingsFromMon() {
    m_settingsValid = false;
}

} // namespace WingletUI
