#include "saomonitor.h"
#include <QDebug>
#include "winglet-ui/hardware/canardinterface.h"
#include "winglet-ui/hardware/sao/canard_constants.h"
#include "winglet-ui/hardware/sao/smbus.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

namespace WingletUI {

SAOMonitor::SAOMonitor(QThread *ownerThread)
{
    moveToThread(ownerThread);

    canard = new CanardInterface(this);

#if NO_HARDWARE
    // Not running on hardware, we don't get an i2c bus
    // Needed so this doesn't start writing garbage to your laptop's SMBus and break your trackpad
    i2cFd = -1;
    errno = ENODEV;
#else
    i2cFd = open(SAO_I2C_BUS, O_RDWR);
#endif

    if (i2cFd >= 0) {
        // Got the bus, we can start running things
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(timerCallback()));
        timer->start(timerRateMs);

        saoitf_state.fd = i2cFd;
        saoitf_state.cb_arg = (void*) this;
        saoitf_state.discovery_cb = &SAOMonitor::discoveryHandler;
        saoitf_state.reset_cb = &SAOMonitor::resetHandler;
        saoitf_host_reset(&saoitf_state);
    }
    else {
        qWarning("SAOMonitor: Failed to open I2C bus (%d)", errno);
    }
}

SAOMonitor::~SAOMonitor()
{
    if (i2cFd >= 0)
        close(i2cFd);
    delete timer;
    delete canard;
}

bool SAOMonitor::discoveryHandler(void* arg, uint8_t addr, uint16_t vid, uint16_t pid, uint16_t fw_version) {
    SAOMonitor* inst = (SAOMonitor*)arg;
    if (vid == CANARD_VID && pid == CANARD_PID && fw_version == CANARD_FW_VERSION) {
        // Canard board
        inst->canard->address = addr;
        inst->canard->commt_setConnected(true);
        return true;
    }
    return false;
}

void SAOMonitor::resetHandler(void* arg) {
    SAOMonitor* inst = (SAOMonitor*)arg;
    inst->canard->commt_setConnected(false);
}

void SAOMonitor::timerCallback()
{
    saoitf_scan_bus(&saoitf_state);
    if (canard->connected()) {
        canard->commt_ping();
    }
}

// ========================================
// SMBus Communication
// ========================================

bool SAOMonitor::commt_readWord(uint8_t addr, uint8_t cmd, uint16_t& wordOut, bool silenceErrs) {
    int32_t ret = ioctl(i2cFd, I2C_SLAVE, addr);
    if (ret < 0) {
        qWarning("SAOMonitor: Failed to set I2C addr to 0x%02X (err: %d)", addr, errno);
        return false;
    }

    ret = i2c_smbus_read_word_data(i2cFd, cmd);
    if (ret < 0) {
        if (!silenceErrs)
            qWarning("SAOMonitor::commt_readWord: SMBus operation failed (err: %d)", errno);
        return false;
    }

    wordOut = ret;
    return true;
}

bool SAOMonitor::commt_readByte(uint8_t addr, uint8_t cmd, uint8_t& byteOut) {
    int ret = ioctl(i2cFd, I2C_SLAVE, addr);
    if (ret < 0) {
        qWarning("SAOMonitor: Failed to set I2C addr to 0x%02X (err: %d)", addr, ret);
        return false;
    }

    ret = i2c_smbus_read_byte_data(i2cFd, cmd);
    if (ret < 0) {
        qWarning("SAOMonitor::commt_readByte: SMBus operation failed (err: %d)", errno);
        return false;
    }

    byteOut = ret;
    return true;
}

bool SAOMonitor::commt_readBlockKnownLen(uint8_t addr, uint8_t cmd, uint8_t len, uint8_t* buf_out) {
    if (len > I2C_SMBUS_BLOCK_MAX) {
        qWarning("SAOMonitor::commt_readBlockKnownLen: Length (%d) greater than block max. Ignoring request", len);
        return false;
    }

    int ret = ioctl(i2cFd, I2C_SLAVE, addr);
    if (ret < 0) {
        qWarning("SAOMonitor: Failed to set I2C addr to 0x%02X (err: %d)", addr, ret);
        return false;
    }

    ret = i2c_smbus_read_block_data_exp_len(i2cFd, cmd, len, buf_out);
    if (ret != 0) {
        qWarning("SAOMonitor::commt_readBlockKnownLen: SMBus operation failed (err: %d)", errno);
        return false;
    }
    return true;
}



bool SAOMonitor::commt_basicCmd(uint8_t addr, uint8_t cmd) {
    int ret = ioctl(i2cFd, I2C_SLAVE, addr);
    if (ret < 0) {
        qWarning("SAOMonitor: Failed to set I2C addr to 0x%02X (err: %d)", addr, ret);
        return false;
    }

    ret = i2c_smbus_write_byte(i2cFd, cmd);
    if (ret < 0) {
        qWarning("SAOMonitor::commt_basicCmd: SMBus operation failed (err: %d)", errno);
        return false;
    }
    return true;
}

bool SAOMonitor::commt_writeByte(uint8_t addr, uint8_t cmd, uint8_t byte) {
    int ret = ioctl(i2cFd, I2C_SLAVE, addr);
    if (ret < 0) {
        qWarning("SAOMonitor: Failed to set I2C addr to 0x%02X (err: %d)", addr, ret);
        return false;
    }

    ret = i2c_smbus_write_byte_data(i2cFd, cmd, byte);
    if (ret < 0) {
        qWarning("SAOMonitor::commt_writeByte: SMBus operation failed (err: %d)", errno);
        return false;
    }
    return true;
}

bool SAOMonitor::commt_writeBlock(uint8_t addr, uint8_t cmd, uint8_t len, const uint8_t* buf) {
    if (len > I2C_SMBUS_BLOCK_MAX) {
        qWarning("SAOMonitor::commt_writeBlock: Length (%d) greater than block max. Ignoring request", len);
        return false;
    }

    int ret = ioctl(i2cFd, I2C_SLAVE, addr);
    if (ret < 0) {
        qWarning("SAOMonitor: Failed to set I2C addr to 0x%02X (err: %d)", addr, ret);
        return false;
    }

    ret = i2c_smbus_write_block_data(i2cFd, cmd, len, buf);
    if (ret < 0) {
        qWarning("SAOMonitor::commt_writeBlock: SMBus operation failed (err: %d)", errno);
        return false;
    }
    return true;
}


} // namespace WingletUI
