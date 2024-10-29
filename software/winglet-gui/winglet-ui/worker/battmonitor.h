#ifndef WINGLETUI_BATTMONITOR_H
#define WINGLETUI_BATTMONITOR_H

#include <QObject>
#include <QTimer>

namespace WingletUI {

class BattMonitor : public QObject
{
    Q_OBJECT
public:
    explicit BattMonitor(QThread *ownerThread);
    ~BattMonitor();

    enum BattState { BATT_UNKNOWN, BATT_DISCHARGING, BATT_CHARGING };
    BattState battState() { return m_battState; }
    int percentage() { return m_percentage; }

signals:
    void battStateChanged(int state, int percentage);

private slots:
    void timerCallback();

private:
    QTimer *timer = NULL;
    const int timerRateMs = 1000;

    BattState readBattStatus();
    int readBattCapacity();

    void reportNewBattState(BattState state, int percentage);
    BattState m_battState = BATT_UNKNOWN;
    int m_percentage = 0;
};

} // namespace WingletUI

#endif // WINGLETUI_BATTMONITOR_H
