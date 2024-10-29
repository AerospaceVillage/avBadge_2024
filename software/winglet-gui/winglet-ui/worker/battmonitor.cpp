#include "battmonitor.h"
#include <QFile>
#include <QTextStream>

namespace WingletUI {

#define SYSFS_BAT_PREFIX "/sys/class/power_supply/battery"

BattMonitor::BattMonitor(QThread *ownerThread)
{
    moveToThread(ownerThread);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerCallback()));
    timer->start(timerRateMs);
    timerCallback();
}

BattMonitor::~BattMonitor()
{
    delete timer;
}

void BattMonitor::reportNewBattState(BattState state, int percentage)
{
    if (state == m_battState && percentage == m_percentage) {
        return;
    }

    m_battState = state;
    m_percentage = percentage;
    emit battStateChanged(state, percentage);
}

BattMonitor::BattState BattMonitor::readBattStatus()
{
    QFile statusFile(SYSFS_BAT_PREFIX "/status");
    if(!statusFile.open(QIODevice::ReadOnly)) {
        return BATT_UNKNOWN;
    }

    BattState state = BATT_UNKNOWN;
    QTextStream in(&statusFile);
    if (!statusFile.atEnd()) {
        QString status = in.readLine();
        if (status == "Charging" || status == "Full") {
            state = BATT_CHARGING;
        }
        else if (status == "Not charging") {
            state = BATT_DISCHARGING;
        }
        // If not, fallthrough to unknown
    }

    statusFile.close();

    return state;
}

int BattMonitor::readBattCapacity()
{
    QFile statusFile(SYSFS_BAT_PREFIX "/capacity");
    if(!statusFile.open(QIODevice::ReadOnly)) {
        return -1;
    }

    int capacity = -1;
    QTextStream in(&statusFile);
    if (!statusFile.atEnd()) {
        QString status = in.readLine();
        bool okay;
        capacity = status.toInt(&okay);
        if (!okay) {
            capacity = -1;
        }
    }

    statusFile.close();

    return capacity;
}

void BattMonitor::timerCallback()
{
    auto state = readBattStatus();
    int capacity = 0;
    if (state == BATT_CHARGING || state == BATT_DISCHARGING) {
        capacity = readBattCapacity();
        // If negative returned, we failed to read capcity, fall back to unknown state
        if (capacity < 0) {
            state = BATT_UNKNOWN;
            capacity = 0;
        }
    }

    reportNewBattState(state, capacity);
}

} // namespace WingletUI
