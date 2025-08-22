#ifndef WINGLETUI_ADSBRECEIVER_H
#define WINGLETUI_ADSBRECEIVER_H

#include "winglet-ui/worker/gpsreceiver.h"
#include "abstractsocketworker.h"
#include <QDateTime>
#include <QMutex>
#include <cmath>

namespace WingletUI {

struct Aircraft {
    int messageType = 0;        /* Message recieved type */
    quint32 icao24 = 0;        /* ADSHex ** Key ID for building array ** */
    QString callSign = "-----"; /* Flight number */
    int alt = 0;                /* Altitude */
    float lat = 0 ;             /* Latitude */
    float lon = 0;              /* Longitude */
    int track = 0;              /* Angle of flight. */
    QDateTime timestamp;        /* The date/time the message was received */
    int squawk = 0;             /* squawk */
    int gndSpeed = 0;           /* Ground Speed */
    float planeTrack = 0;
    bool isOnGround = false;

    float distance = nanf("");  // Initializes distnace as a quiet NaN       // Have distance calculate on receipt of message (vice every paint)
    float bearing = nanf("");   // Initialize bearing as a quiet NaN

    bool callSignValid = false;
    bool altValid = false;
    bool latValid = false;
    bool lonValid = false;
    bool trackValid = false;
    bool squawkValid = false;
    bool gndSpeedValid = false;
    bool planeTrackValid = false;
    bool onGroundValid = false;
};

class ADSBReceiver : public WingletUI::AbstractSocketWorker
{
    Q_OBJECT
public:
    ADSBReceiver(QThread *ownerThread);

    bool connected() {return m_connected;}
    QMap<quint32, Aircraft> airspace();

signals:
    void connectionStateChanged(bool connected);

protected:
    void handleConnectionEvent(bool connected) override;
    void handleLine(const QString &line) override;
    float distanceEarth(double lat2d, double lon2d);
    float get_bearing(float lat, float lon);

private slots:
    void clearStalePlanesCallback();

private:
    void clearStalePlanesUnderLock();
    bool m_connected = false;
    QMutex state_mutex;
    QMap<quint32, Aircraft> m_airspace;
    QTimer *timer;
    GPSReading currentGPS;
};

} // namespace WingletUI

#endif // WINGLETUI_ADSBRECEIVER_H
