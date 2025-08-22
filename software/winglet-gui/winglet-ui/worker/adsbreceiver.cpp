#include "adsbreceiver.h"
#include "wingletgui.h"
#include <cmath>

#define earthRadiusKm 6371.0

namespace WingletUI {

ADSBReceiver::ADSBReceiver(QThread *ownerThread):
    AbstractSocketWorker(ownerThread)
{
    // Load the last latitude/longitude readings in
    currentGPS = GPSReading(WingletGUI::inst->settings.lastLatitude(),
                            WingletGUI::inst->settings.lastLongitude(), false);

    start("localhost", 30003);
    timer = new QTimer(this);
    timer->setInterval(3000);
    connect(timer, SIGNAL(timeout()), this, SLOT(clearStalePlanesCallback()));
    timer->start();
}

void ADSBReceiver::handleConnectionEvent(bool connected)
{
    if (!connected) {
        // GPS Disconnected

        state_mutex.lock();
        m_airspace.clear();
        state_mutex.unlock();
    }

    m_connected = connected;
    emit connectionStateChanged(connected);
}

QMap<quint32, Aircraft> ADSBReceiver::airspace()
{
    state_mutex.lock();
    QMap<quint32, Aircraft> airspaceCopy = m_airspace;
    state_mutex.unlock();

    return airspaceCopy;
}

void ADSBReceiver::handleLine(const QString &line) {
    QStringList query = line.trimmed().split(',');

    if (query.count() != 22)
        return;
    if (query[0] != "MSG")
        return;

    bool okay;
    quint32 icao24 = query[4].toUInt(&okay, 16);
    if (!okay)
        return;

    state_mutex.lock();

    auto entry = m_airspace.find(icao24);
    if (entry == m_airspace.end())
        entry = m_airspace.insert(icao24, {});
    entry->icao24 = icao24;

    uint messageType = query[1].toUInt(&okay);
    if (!okay)
        goto unlock;

    entry->messageType = messageType;

    // Set current timestamp if time not found
    if(query[8] == ""){
        QDateTime currentTime = QDateTime::currentDateTimeUtc();
        entry->timestamp = currentTime;
    } else  //Decode timestamp when supported
    {
        QString strTime = QString(query[8]+" "+query[9]);
        QDateTime last_msg  = QDateTime::fromString(strTime, "yyyy/MM/dd HH:mm:ss");
        last_msg.setTimeSpec(Qt::UTC);
        entry->timestamp = last_msg;
    }

    if (messageType == 1) {
        entry->callSign = query[10];
        entry->callSignValid = true;
    }
    if(messageType == 2 || messageType == 3 || messageType == 5 || messageType == 6 || messageType == 7){
       int alt = query[11].toInt(&okay);
       if (!okay)
           goto unlock;

       entry->alt = alt;
       entry->altValid = true;
    }
    if(messageType == 2 || messageType == 4){
        int gndSpeed = query[12].toInt(&okay);
        if (!okay)
            goto unlock;

        int planeTrack = query[13].toFloat(&okay);
        if (!okay)
            goto unlock;

        entry->gndSpeed = gndSpeed;
        entry->planeTrack = planeTrack;
        entry->gndSpeedValid = true;
        entry->planeTrackValid = true;
    }
    if(messageType == 2 || messageType == 3){
        float lat = query[14].toFloat(&okay);
        if (!okay)
            goto unlock;

        float lon = query[15].toFloat(&okay);
        if (!okay)
            goto unlock;

        entry->lat = lat;
        entry->lon = lon;
        entry->lonValid = true;
        entry->latValid = true;

        entry->distance = distanceEarth(lat, lon);  //Update distance upon reciept of new message
        entry->bearing = get_bearing(lat, lon);     //Update bearing upon reciept of new message
    }
    if(messageType == 6){
        int squawk = query[17].toInt(&okay);
        if (!okay)
            goto unlock;

        entry->squawk = squawk;
        entry->squawkValid = true;
    }
    if(messageType == 2 || messageType == 3 || messageType == 5 || messageType == 6 || messageType == 7 || messageType == 8){
        if(query[21] == "1"){
            entry->isOnGround = true;
        }
        else if (query[21] == "0"){
            entry->isOnGround = false;
        }
        entry->onGroundValid = true;
    }


unlock:
    clearStalePlanesUnderLock();
    state_mutex.unlock();
}

void ADSBReceiver::clearStalePlanesCallback() {
    state_mutex.lock();
    clearStalePlanesUnderLock();
    state_mutex.unlock();
}

void ADSBReceiver::clearStalePlanesUnderLock()
{
    int decayTime = WingletGUI::inst->settings.adsbDecayTimeSec();

    QDateTime currentTime = QDateTime::currentDateTimeUtc();
    for (auto it = m_airspace.begin(); it != m_airspace.end();) {
        qint64 diff = currentTime.secsTo(it.value().timestamp);
        if(abs(diff) > decayTime) {
            it = m_airspace.erase(it);
        }
        else {
            it++;
        }
    }
}

float ADSBReceiver::distanceEarth(double lat2d, double lon2d) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = currentGPS.latitude * M_PI / 180;
  lon1r = currentGPS.longitude * M_PI / 180;
  lat2r = lat2d * M_PI / 180;
  lon2r = lon2d * M_PI / 180;
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v)) * 0.539957;
}

float ADSBReceiver::get_bearing(float lat, float lon){
    /// center location scope
    float startLat = currentGPS.latitude * (M_PI/180);
    float startLon = currentGPS.longitude * (M_PI/180);

    /// location of plane
    float endLat = lat * (M_PI/180);
    float endLon = lon * (M_PI/180);

    float dLong = endLon - startLon;

    float dPhi = log(tan(endLat/2.0 + M_PI/4.0)/tan(startLat/2.0 + M_PI/4.0));
    if(abs(dLong) > M_PI){
        if (dLong > 0.0){
            dLong = -(2.0 * M_PI - dLong);
        } else {
            dLong = (2.0 * M_PI + dLong);
        }
    }
    float bearing = fmod(((atan2(dLong, dPhi)*(180/M_PI)) + 360.0), 360.0);

    return bearing;
}

} // namespace WingletUI
