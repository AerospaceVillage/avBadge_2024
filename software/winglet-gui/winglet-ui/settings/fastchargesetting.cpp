#include "fastchargesetting.h"
#include "appsettings.h"

#include <QFile>
#include <QTextStream>

namespace WingletUI {

FastChargeSetting::FastChargeSetting(AppSettings *parent)
    : AbstractBoolSetting(parent, "3A Fast Charge"), settings(parent) {

    setValue(parent->fastCharge3APersistent());
}

bool FastChargeSetting::readRegVal(uint8_t *regOut) {
    QFile f;
    f.setFileName("/sys/class/axp/axp_reg");
    if (!f.open(QFile::WriteOnly | QFile::Text)) {
        return false;
    }
    f.write("8B\n");
    f.close();

    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }
    QTextStream in(&f);
    QString reg_val_str = in.readLine();
    f.close();

    QString reg_prefix = "REG[0x8b]=0x";
    if (!reg_val_str.startsWith(reg_prefix)) {
        return false;
    }
    reg_val_str = reg_val_str.mid(reg_prefix.size());

    bool decode_okay;
    int reg_val = reg_val_str.toInt(&decode_okay, 16);
    if (!decode_okay) {
        return false;
    }

    if (reg_val < 0 || reg_val > 0xFF) {
        return false;
    }

    *regOut = reg_val;
    return true;
}

#define CURRENT_3A 0b101111
#define CURRENT_1A 0b010000

int FastChargeSetting::value() const {

    uint8_t reg_val;
    if (!readRegVal(&reg_val)) {
        return -1;
    }

    uint8_t charge_current = reg_val & 0x3F;
    if (charge_current == CURRENT_3A) {
        return 1;   // 3A charge
    }
    else if (charge_current == CURRENT_1A) {
        return 0;   // 1A charge (default)
    }
    else {
        return -1;  // Unknown current? Fall back to tristate
    }
}

void FastChargeSetting::setValue(bool val) {
    uint8_t newVal;
    if (!readRegVal(&newVal)) {
        qWarning("Failed to set charge current: Failed to read initial value");
        return;
    }
    newVal &= 0xC0;  // Mask off bits we'll replace

    if (val) {
        newVal |= CURRENT_3A;
    }
    else {
        newVal |= CURRENT_1A;
    }

    // Format for writing
    uint16_t regWrite = 0x8B00 | newVal;
    QString regWriteStr = QString("%1\n").arg(regWrite, 0, 16);
    QByteArray regWriteBytes = regWriteStr.toLatin1();

    // Write out to hardware
    QFile f;
    f.setFileName("/sys/class/axp/axp_reg");
    if (!f.open(QFile::WriteOnly | QFile::Text)) {
        qWarning("Failed to set charge current: Failed to open register for writing");
        return;
    }
    f.write(regWriteBytes);
    f.close();

    // Save setting to survive across reboots
    if (settings->fastCharge3APersistent() != val) {
        settings->setFastCharge3APersistent(val);
    }

    // Notify UI to refresh this setting
    reportChanged();
}

} // namespace WingletUI
