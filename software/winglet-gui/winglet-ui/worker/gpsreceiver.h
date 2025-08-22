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

struct Satellite {
    // From --GSV, both GPS and Beidou
    int numSatField = 0;
    qint32  svid = 0;
    int     elev = 0;
    int     azim = 0;
    int     cno = 0;
    QDateTime timestamp;
};

// Right now this *should* be acting the same as GPSReading. Not sure why it consistently

struct GPSConstellation {
    GPSConstellation(): satsTracked(0), msl(0), posStatus(0), pdop(0), hdop(0), vdop(0) {}
    //GPSConstellation(const QStringList &nmeaFields);

    QMap<quint32, Satellite> m_ephemeris;

    // From GGA
    int     satsTracked = 0;
    float   msl = 0;

    // From GSA
    int     posStatus = 0;
    float   pdop = 0;
    float   hdop = 0;
    float   vdop = 0;
}; 

class GPSReceiver : public WingletUI::AbstractSocketWorker
{
    Q_OBJECT
public:
    enum GPSState { GPS_DISCONNECTED, GPS_NO_LOCK, GPS_LOCKED };
    enum PosState { POS_UNKNOWN, POS_INVALID, POS_2D, POS_3D };
    Q_ENUM(PosState) // Enables meta-object system support for MyEnum

    GPSReceiver(QThread *ownerThread);
    GPSReading lastReading();
    GPSState state() { return m_state; }
    QMap<quint32, Satellite> getEphemeris();
    GPSConstellation getConstellation();

    void updateGPSConstellation(const QStringList &nmeaFields);



signals:
    void gpsUpdated(WingletUI::GPSReading state);
    void stateUpdated(int state);

protected:
    void handleConnectionEvent(bool connected) override;
    void handleLine(const QString &line) override;

private slots:
    void clearStaleSatsCallback();

private:
    void clearStaleSatsUnderLock();
    QMutex state_mutex;
    GPSState m_state = GPS_DISCONNECTED;
    GPSReading m_reading;

    QTimer *timer;
    GPSConstellation m_constellation;

};

} // namespace WingletUI

Q_DECLARE_METATYPE(WingletUI::GPSReading);

#endif // WINGLETUI_GPSRECEIVER_H
