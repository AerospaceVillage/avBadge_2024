#include "gpsreceiver.h"

namespace WingletUI {

GPSReceiver::GPSReceiver(QThread *ownerThread):
    AbstractSocketWorker(ownerThread)
{
    // Host to connect to for socket
    start("localhost", 2947);
}

void GPSReceiver::handleConnectionEvent(bool connected)
{
    if (connected) {
        // GPS Connected
        // We aren't refreshing state here, wait until first NMEA message comes in
        writeData("?WATCH={\"enable\":true,\"nmea\":true}\n");
    }
    else {
        // GPS Disconnected

        bool oldReadingValid = m_reading.valid;
        GPSState oldState = m_state;

        state_mutex.lock();
        m_reading = GPSReading();
        m_state = GPS_DISCONNECTED;
        state_mutex.unlock();

        if (oldReadingValid)
            emit gpsUpdated(GPSReading());
        if (oldState != m_state)
            emit stateUpdated(m_state);
    }
}

GPSReading GPSReceiver::lastReading() {
    state_mutex.lock();
    GPSReading state = m_reading;
    state_mutex.unlock();
    return state;
}

void GPSReceiver::handleLine(const QString &line) {
    QStringList fields = line.trimmed().split(',');

    if (fields.length() < 1)
        return;

    const QString& type = fields.at(0);

    if (type == "$GPRMC") {
        // GPS Transit Data Message
        GPSReading oldReading = m_reading;
        GPSState oldState = m_state;\

        state_mutex.lock();
        m_reading = GPSReading(fields);
        m_state = m_reading.valid ? GPS_LOCKED : GPS_NO_LOCK;
        state_mutex.unlock();

        if (oldState != m_state)
            emit stateUpdated(m_state);
        if (oldReading != m_reading)
            emit gpsUpdated(m_reading);
    }
}


GPSReading::GPSReading(const QStringList &nmeaFields): GPSReading()  {
    if (nmeaFields.length() != 12)
        return;

    bool okay;
    int day = nmeaFields[9].mid(0, 2).toInt(&okay);
    if (!okay)
        return;

    int month = nmeaFields[9].mid(2, 2).toInt(&okay);
    if (!okay)
        return;

    int year = nmeaFields[9].mid(4, 2).toInt(&okay) + 2000;
    if (!okay)
        return;

    int hour = nmeaFields[1].mid(0, 2).toInt(&okay);
    if (!okay)
        return;

    int min = nmeaFields[1].mid(2, 2).toInt(&okay);
    if (!okay)
        return;

    int sec = nmeaFields[1].mid(4, 2).toInt(&okay);
    if (!okay)
        return;

    QDate date(year, month, day);
    QTime time(hour, min, sec);

    timestamp = QDateTime(date, time, Qt::UTC);

    if (nmeaFields[2] != "A")
        return;  // No lock, not valid message


    int latDeg = nmeaFields[3].left(2).toInt(&okay);
    if (!okay)
        return;

    float latMin = nmeaFields[3].right(7).toFloat(&okay);
    if (!okay)
        return;

    float latSign;
    if (nmeaFields[4] == "N")
        latSign = 1;
    else if (nmeaFields[4] == "S")
        latSign = -1;
    else
        return; // Invalid sign


    int lonDeg = nmeaFields[5].left(3).toInt(&okay);
    if (!okay)
        return;

    float lonMin = nmeaFields[5].right(7).toFloat(&okay);
    if (!okay)
        return;

    float lonSign;
    if (nmeaFields[6] == "E")
        lonSign = 1;
    else if (nmeaFields[6] == "W")
        lonSign = -1;
    else
        return; // Invalid sign

    float knotsTemp = nmeaFields[7].toFloat(&okay);
    if (!okay)
        return;

    latitude = latSign * (latDeg + (latMin/60.0));
    longitude = lonSign*(lonDeg + (lonMin/60.0));
    speedKnots = knotsTemp;
    valid = true;
}

bool GPSReading::operator==(const GPSReading& rhs) const
{
     if (!valid) {
         return !rhs.valid;
     }

     return latitude == rhs.latitude &&
             longitude == rhs.longitude &&
             speedKnots && rhs.speedKnots &&
             timestamp == rhs.timestamp;
}

} // namespace WingletUI
