#ifndef WINGLETUI_GPSRECEIVER_H
#define WINGLETUI_GPSRECEIVER_H

#include "abstractsocketworker.h"
#include <QDateTime>
#include <QMutex>

namespace WingletUI {

struct GPSReading {
    GPSReading(): valid(false), latitude(0), longitude(0), speedKnots(0), timestamp() {}
    GPSReading(float lat, float lon, bool valid = true): valid(valid), latitude(lat), longitude(lon), speedKnots(0), timestamp() {}
    GPSReading(const QStringList &nmeaFields);

    bool valid;
    float latitude;
    float longitude;
    float speedKnots;
    QDateTime timestamp;

    bool operator==(const GPSReading& rhs) const;
    bool operator!=(const GPSReading& rhs) const {return !operator==(rhs);}
};

class GPSReceiver : public WingletUI::AbstractSocketWorker
{
    Q_OBJECT
public:
    enum GPSState { GPS_DISCONNECTED, GPS_NO_LOCK, GPS_LOCKED };

    GPSReceiver(QThread *ownerThread);
    GPSReading lastReading();
    GPSState state() { return m_state; }

signals:
    void gpsUpdated(WingletUI::GPSReading state);
    void stateUpdated(int state);

protected:
    void handleConnectionEvent(bool connected) override;
    void handleLine(const QString &line) override;

private:
    QMutex state_mutex;
    GPSState m_state = GPS_DISCONNECTED;
    GPSReading m_reading;
};

} // namespace WingletUI

Q_DECLARE_METATYPE(WingletUI::GPSReading);

#endif // WINGLETUI_GPSRECEIVER_H
