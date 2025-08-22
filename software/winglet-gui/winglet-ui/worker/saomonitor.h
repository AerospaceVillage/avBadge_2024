#ifndef WINGLETUI_SAOMONITOR_H
#define WINGLETUI_SAOMONITOR_H

#include <QObject>
#include <QTimer>
#include "winglet-ui/hardware/sao/saoitf_host.h"

#define SAO_I2C_BUS "/dev/i2c-0"

namespace WingletUI {

class CanardInterface;

class SAOMonitor : public QObject
{
    Q_OBJECT
public:
    explicit SAOMonitor(QThread *ownerThread);
    ~SAOMonitor();
    CanardInterface *canard;

protected:
    friend class CanardInterface;
    // SMBus Communication
    bool commt_basicCmd(uint8_t addr, uint8_t cmd);
    bool commt_readWord(uint8_t addr, uint8_t cmd, uint16_t& wordOut, bool silenceErrs = false);
    bool commt_readByte(uint8_t addr, uint8_t cmd, uint8_t& byteOut);
    bool commt_readBlockKnownLen(uint8_t addr, uint8_t cmd, uint8_t len, uint8_t* buf_out);
    bool commt_writeByte(uint8_t addr, uint8_t cmd, uint8_t byte);
    bool commt_writeBlock(uint8_t addr, uint8_t cmd, uint8_t len, const uint8_t* buf);

private slots:
    void timerCallback();

private:
    static bool discoveryHandler(void* arg, uint8_t addr, uint16_t vid, uint16_t pid, uint16_t fw_version);
    static void resetHandler(void* arg);

    int i2cFd = -1;
    QTimer *timer = NULL;
    const int timerRateMs = 1000;
    saoitf_host_state_t saoitf_state = {};
};

} // namespace WingletUI

#endif // WINGLETUI_SAOMONITOR_H
