#include "canardinterface.h"
#include "winglet-ui/settings/canardsettings.h"
#include "sao/saoitf_constants.h"

#include <QDebug>
#include "wingletgui.h"

namespace WingletUI {

CanardInterface::CanardInterface(SAOMonitor *parent): QObject{parent}, m_mon(parent)
{
    // Settings needs to be created as part of winglet gui inst so it lives in the main thread for UI stuff;
    settings = WingletGUI::inst->canardSettings;
    settings->setInterface(this);  // Assign this to the canard interface settings
    connect(this, SIGNAL(triggerLoadSettings()), settings, SLOT(loadSettingsFromMon()));
    connect(this, SIGNAL(triggerInvalidateSettings()), settings, SLOT(invalidateSettingsFromMon()));

#if NO_HARDWARE
    m_connected = true;
#endif
}

CanardInterface::~CanardInterface(){

}

void CanardInterface::commt_setConnected(bool connected) {
    if (connected == m_connected)
        return;

    if (connected) {
        // Try 3 times to read out settings
        bool okay = false;
        for (int i = 0; i < 3; i++) {
            if (!commt_readoutSettings(true)) {
                qWarning("CanardInterface::commt_setConnected: Failed to initialize state from device (retry %d)", i + 1);
            }
            else {
                okay = true;
                break;
            }
        }
        if (!okay)
            return;
    }

    m_connected = connected;
    if (connected) {
        emit triggerLoadSettings(QPrivateSignal());
    }
    else {
        emit triggerInvalidateSettings(QPrivateSignal());
    }
    emit connectionStateChanged(m_connected);
}

bool CanardInterface::commt_setBadgeControl(bool saoUnderBadgeCtrl) {
#if NO_HARDWARE
    // Show demo if in no hardware mode
    m_cachedVolume = 60;
    m_cachedRadioState.mode = MODE_AIRBAND;
    m_cachedRadioState.presetMode = false;
    m_cachedRadioState.tuning.freq = 121500;
    return true;
#endif

    if (!commt_setCtrlEnable(saoUnderBadgeCtrl)) {
        return false;
    }

    if (saoUnderBadgeCtrl) {
        // Read out state if we're entering badge control
        if (!commt_readoutRadioState()) {
            return false;
        }
        if (!commt_readoutVolume()) {
            return false;
        }
    }
    return true;
}

// ========================================
// UI Thread Functions
// ========================================

bool CanardInterface::setBadgeControlMode(bool saoUnderBadgeCtrl) {
    bool result;
    QMetaObject::invokeMethod(this, "commt_setBadgeControl", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result), Q_ARG(bool, saoUnderBadgeCtrl));
    return result;
}

CanardInterface::CanardFreq CanardInterface::getFreq(){

    // TODO: This should be fixed
    CanardFreq tmp;
    tmp.wholeFreq = m_cachedRadioState.tuning.freq / 1000;
    tmp.decFreq = m_cachedRadioState.tuning.freq % 1000;

    return tmp;
}

bool CanardInterface::setFreq(CanardInterface::CanardFreq values){
    RadioState tmpState;
    tmpState.presetMode = false;
    tmpState.mode = MODE_AIRBAND;
    tmpState.tuning.freq = values.wholeFreq * 1000 + (values.decFreq % 1000);
    setRadioState(tmpState);
    return true;
}

void CanardInterface::setRadioState(const RadioState& newState) {
    uint8_t mode = 0;
    if (newState.presetMode)
        mode |= CANARD_RADIO_STATE_PRESET_MODE_MASK;
    mode |= (((uint8_t)newState.mode) & CANARD_RADIO_MODE_MASK) << CANARD_RADIO_MODE_SHIFT;

    uint32_t freq_preset = newState.tuning.freq;

    // Generate output data
    QByteArray cmd(5, 0);
    cmd[0] = mode;
    cmd[1] = freq_preset & 0xFF;
    cmd[2] = (freq_preset >> 8) & 0xFF;
    cmd[3] = (freq_preset >> 16) & 0xFF;
    cmd[4] = (freq_preset >> 24) & 0xFF;

    m_cachedRadioState = newState;
    QMetaObject::invokeMethod(this, "commt_setRadioState", Q_ARG(QByteArray, cmd));
}

void CanardInterface::setVolume(uint8_t volume) {
    m_cachedVolume = volume;
    QMetaObject::invokeMethod(this, "commt_setVolume", Q_ARG(uint, volume));
}

bool CanardInterface::loadSettingsFromDevice(uint8_t* settingsOut) {
    if (!m_connected) {
        return false;
    }
    memcpy(settingsOut, m_cachedSettings, sizeof(m_cachedSettings));
    return true;
}

bool CanardInterface::saveSettingsToDevice(const uint8_t* newSettings) {
    if (!m_connected) {
        return false;
    }

    QByteArray newSettingsArr((char*) newSettings, CANARD_SETTINGS_LEN);
    memcpy(m_cachedSettings, newSettings, sizeof(m_cachedSettings));
    bool result;
    QMetaObject::invokeMethod(this, "commt_writeSettings", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result), Q_ARG(QByteArray, newSettingsArr));
    return result;
}

// ========================================
// SMBus Operation Wrappers
// ========================================

void CanardInterface::commt_ping() {
    if (!m_connected)
        return;

    for (int i = 0; i < 5; i++) {
        uint16_t magic = 0;
        if (!m_mon->commt_readWord(address, SAOITF_CMD_MAGIC, magic, true)) {
            continue;
        }

        if (magic != SAOITF_MAGIC_WORD) {
            continue;
        }
        return;
    }

    // Couldn't ping, mark disconnected
    commt_setConnected(false);
}

bool CanardInterface::commt_readoutRadioState() {
    if (!m_connected) {
        return false;
    }

    uint8_t data[CANARD_RADIO_STATE_LEN];
    if (!m_mon->commt_readBlockKnownLen(address, CANARD_CMD_GET_RADIO_STATE, sizeof(data), data)) {
        qWarning("CanardInterface::commt_readoutRadioState: Failed to get radio state from device");
        return false;
    }

    bool preset_mode = data[0] & CANARD_RADIO_STATE_PRESET_MODE_MASK;
    uint8_t radio_mode = (data[0] >> CANARD_RADIO_MODE_SHIFT) & CANARD_RADIO_MODE_MASK;
    uint32_t freq_preset = 0;
    freq_preset |= data[4];
    freq_preset <<= 8;
    freq_preset |= data[3];
    freq_preset <<= 8;
    freq_preset |= data[2];
    freq_preset <<= 8;
    freq_preset |= data[1];

    if (radio_mode != MODE_FM && radio_mode != MODE_AIRBAND) {
        qWarning("CanardInterface::commt_readoutRadioState: Invalid radio mode reported");
        return false;
    }
    m_cachedRadioState.mode = (RadioMode) radio_mode;
    m_cachedRadioState.presetMode = preset_mode;
    m_cachedRadioState.tuning.freq = freq_preset;
    return true;
}

bool CanardInterface::commt_readoutVolume() {
    if (!m_connected) {
        return false;
    }

    return m_mon->commt_readByte(address, CANARD_CMD_GET_VOLUME, m_cachedVolume);
}
bool CanardInterface::commt_readoutSettings(bool ignoreNoConnected) {
    if (!m_connected && !ignoreNoConnected) {
        return false;
    }

    size_t copy_len = 30;
    if (!m_mon->commt_writeByte(address, CANARD_CMD_START_CONFIG_READ, copy_len)) {
        qWarning("CanardInterface::commt_readoutSettings: Failed to start settings readout");
        return false;
    }

    static_assert(sizeof(m_cachedSettings) == CANARD_SETTINGS_LEN, "Length mismatch");
    for (int i = 0; i < CANARD_SETTINGS_LEN; i += copy_len) {
        uint8_t expected = copy_len;
        if (expected > (CANARD_SETTINGS_LEN - i))
            expected = (CANARD_SETTINGS_LEN - i);

        if (!m_mon->commt_readBlockKnownLen(address, CANARD_CMD_READ_CONFIG, expected, m_cachedSettings + i)) {
            qWarning("CanardInterface::commt_readoutSettings: Failed to read settings block at %d", i);
            return false;
        }
    }

    return true;
}

void CanardInterface::commt_setRadioState(QByteArray state) {
    if (!m_connected) {
        return;
    }

    if (!m_mon->commt_writeBlock(address, CANARD_CMD_SET_RADIO_STATE, state.size(), (uint8_t*) state.constData())) {
        qWarning("CanardInterface::commt_setRadioState: Failed to commit radio state to device!");
    }
}

void CanardInterface::commt_setVolume(uint volume) {
    if (!m_connected) {
        return;
    }

    if (!m_mon->commt_writeByte(address, CANARD_CMD_SET_VOLUME, volume)) {
        qWarning("CanardInterface::commt_setVolume: Failed to commit volume to device!");
    }
}

bool CanardInterface::commt_writeSettings(QByteArray settings) {
    if (!m_connected) {
        return false;
    }

    if (settings.length() != CANARD_SETTINGS_LEN) {
        qWarning("CanardInterface::commt_writeSettings: Invalid settings length (%d). Refusing to write", settings.size());
        return false;
    }
    size_t copy_len = 30;
    if (!m_mon->commt_basicCmd(address, CANARD_CMD_START_CONFIG_WRITE)) {
        qWarning("CanardInterface::commt_writeSettings: Failed to start settings readout");
        return false;
    }

    const uint8_t* newSettingsPtr = (const uint8_t*) settings.constData();

    static_assert(sizeof(m_cachedSettings) == CANARD_SETTINGS_LEN, "Length mismatch");
    for (int i = 0; i < CANARD_SETTINGS_LEN; i += copy_len) {
        uint8_t xfer_len = copy_len;
        if (xfer_len > (CANARD_SETTINGS_LEN - i))
            xfer_len = (CANARD_SETTINGS_LEN - i);

        if (!m_mon->commt_writeBlock(address, CANARD_CMD_WRITE_CONFIG, xfer_len, newSettingsPtr + i)) {
            qWarning("CanardInterface::commt_writeSettings: Failed to write settings block at %d", i);
            return false;
        }
    }

    uint8_t val;
    if (!m_mon->commt_readByte(address, CANARD_CMD_FINISH_CONFIG_WRITE, val)) {
        qWarning("CanardInterface::commt_writeSettings: Failed to get write status");
        return false;
    }
    if (val != 0) {
        qWarning("CanardInterface::commt_writeSettings: Device reported error code %d", val);
        return false;
    }

    return true;
}

bool CanardInterface::commt_setCtrlEnable(bool ctrlEnable) {
    if (!m_connected) {
        return false;
    }

    if (!m_mon->commt_writeByte(address, CANARD_CMD_SET_CTRL_ENABLE, ctrlEnable ? 1 : 0)) {
        qWarning("CanardInterface::commt_setCtrlEnable: Failed to set ctrl enable");
        return false;
    }
    return true;
}

} // namespace WingletUI
