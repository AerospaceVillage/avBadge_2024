#include "gpsreceiver.h"
#include "wingletgui.h"
#include <QtDebug>

namespace WingletUI {

GPSReceiver::GPSReceiver(QThread *ownerThread):
    AbstractSocketWorker(ownerThread)
{
    // Host to connect to for socket
    start("localhost", 2947);
    // Starting timer for satellite timeout
    timer = new QTimer(this);
    timer->setInterval(3000);
    connect(timer, SIGNAL(timeout()), this, SLOT(clearStaleSatsCallback()));
    timer->start();

    m_constellation = GPSConstellation();
}

void GPSReceiver::handleConnectionEvent(bool connected)
{
    if (connected) {
        // GPS Connected
        // We aren't refreshing state here, wait until first NMEA message comes in
        system("echo -e \"\\$PCAS03,1,1,1,1,1,0,0,0,0,0,,,0,0,,,,0*33\r\n\" > /dev/gnss0");
        writeData("?WATCH={\"enable\":true,\"nmea\":true}\n");
    }
    else {
        // GPS Disconnected

        bool oldReadingValid = m_reading.valid;
        GPSState oldState = m_state;

        state_mutex.lock();
        m_reading = GPSReading();
        m_state = GPS_DISCONNECTED;
        // Just added
        m_constellation = GPSConstellation();
        state_mutex.unlock();

        if (oldReadingValid)
            emit gpsUpdated(GPSReading());
        if (oldState != m_state)
            emit stateUpdated(m_state);
    }
}

QMap<quint32, Satellite> GPSReceiver::getEphemeris()
{
    state_mutex.lock();
    QMap<quint32, Satellite> ephemerisCopy = m_constellation.m_ephemeris;
    state_mutex.unlock();

    return ephemerisCopy;
}

GPSConstellation GPSReceiver::getConstellation()
{
    //This was me trying something out, ignore for now.

    //state_mutex.lock();
    //GPSConstellation conCopy = m_constellation;
    //state_mutex.unlock();
    //return conCopy;
    return m_constellation;
}

GPSReading GPSReceiver::lastReading() {
    state_mutex.lock();
    GPSReading state = m_reading;
    state_mutex.unlock();
    return state;
}

void GPSReceiver::handleLine(const QString &line) {
    QStringList fields = line.trimmed().split(',' , Qt::KeepEmptyParts);

    if (fields.length() < 1)
        return;

    const QString& type = fields.at(0);

    state_mutex.lock();

    // This is all untouched in the code so that I don't screw up getting position
    // from GPRMC. Position state is still works just fine.

    if (type == "$GPRMC") {
        // GPS Transit Data Message
        GPSReading oldReading = m_reading;
        GPSState oldState = m_state;\

        m_reading = GPSReading(fields);
        m_state = m_reading.valid ? GPS_LOCKED : GPS_NO_LOCK;

        if (oldState != m_state)
            emit stateUpdated(m_state);
        if (oldReading != m_reading)
            emit gpsUpdated(m_reading);
    }
    else {
        GPSConstellation oldConstellation = m_constellation;
        updateGPSConstellation(fields);
        //m_constellation = GPSConstellation(fields);
        //qDebug("type not GPRMC"); // This is to let me know that at least GPSConstellation is called.
    }

    // At this point, the pdop hdop and vdop is 0. This is shown in
    // the debug console. Why it is still 0, I have no idea. Everything 

    //qDebug("-- constellation data --");
    //qDebug("pdop: %f", m_constellation.pdop);
    //qDebug("hdop: %f", m_constellation.hdop);
    //qDebug("vdop: %f", m_constellation.vdop);

    //qDebug("-- pos data --");
    //qDebug("lat: %f", m_reading.latitude);
    //qDebug("long: %f", m_reading.longitude);

    clearStaleSatsUnderLock();
    state_mutex.unlock();
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

// At this point, GPSConstellation is working as intended. It pulls data from the NMEA message
// and sorts it properly for all message types. The GPS/BD split also works.

void GPSReceiver::updateGPSConstellation(const QStringList &nmeaFields){

    //Useful (possibly) to print the actual messages being processed when debugging
    //qDebug() << nmeaFields;

    if (nmeaFields[0] == "$GNGGA") {
        bool okay;
        int satsTracked = nmeaFields[7].toInt(&okay);
        if (!okay){
            return;
        }else{
            m_constellation.satsTracked = satsTracked;
        }

        float msl = nmeaFields[9].toFloat(&okay);
        if (!okay){
            return;
        }else{
            m_constellation.msl = msl;
        }
    }

    if (nmeaFields[0] == "$GNGSA") {
        bool okay;
        int posStatus = nmeaFields[2].toInt(&okay);
        if (!okay){
            return;
        }else{
            m_constellation.posStatus = posStatus;
        }

        float pdop = nmeaFields[15].toFloat(&okay);
        if (!okay){
            return;
        }else{
            m_constellation.pdop = pdop;
        }

        float hdop = nmeaFields[16].toFloat(&okay);
        if (!okay){
            return;
        }else{
            m_constellation.hdop = hdop;
        }

        float vdop = nmeaFields[17].toFloat(&okay);
        if (!okay){
            return;
        }else{
            m_constellation.vdop = vdop;
        }

    }

    // We want both BD and GP GSV messages
    // BD SVs will have an offset of 100
    if (nmeaFields[0].right(3) == "GSV") {
        bool okay;
        
        /* Because we don't know how many satellites we'll need to process
        for each GSV message (and because it's also split with GPS/Beidou)
        we need to figure out how many satellites are in the message
        first so we can iterate through the nmeaFields */

        // 5 fields outside of the SVID,ele,az,cn0 list
        int numSatField = ((nmeaFields.size() - 5) / 4);

        for (int i = 1; i <= numSatField; ++i)
        {
            int svOffset = 0;
            if (nmeaFields[0].left(3) == "$BD"){
                svOffset = 100;
            } 

            qint32 currSV = (nmeaFields[i*4].toInt() + svOffset); // Beidou offset added
            auto svSlot = m_constellation.m_ephemeris.find(currSV);
            if (svSlot == m_constellation.m_ephemeris.end())
                svSlot = m_constellation.m_ephemeris.insert(currSV, {});
            svSlot->svid = currSV;

            int elev = nmeaFields[1+(i*4)].toInt(&okay);
            if (!okay)
                return;
            svSlot->elev = elev;

            int azim = nmeaFields[2+(i*4)].toInt(&okay);
            if (!okay)
                return; 
            svSlot->azim = azim;

            int cno = nmeaFields[3+(i*4)].toInt(&okay);
            if (!okay)
                cno = 0;
                // This is captured since the NMEA message may not have a value
                // if the chip is not using this SVID in the solution. 
                // return;
            svSlot->cno = cno;

            QDateTime currentTime = QDateTime::currentDateTimeUtc();
            svSlot->timestamp = currentTime;

        } //endfor
    } // endif
} // end GPSConstellation()

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

void GPSReceiver::clearStaleSatsCallback() {
    state_mutex.lock();
    clearStaleSatsUnderLock();
    state_mutex.unlock();
}

void GPSReceiver::clearStaleSatsUnderLock()
{
    /* decayTime is not going to be variable since if a satellite
    disappears, it's completely gone from the tracking loop.
    Decay will be 10 sec */
    int decayTime = 5;

    // My only other thought is that this might be screwing something up

    QDateTime currentTime = QDateTime::currentDateTimeUtc();
    for (auto it = m_constellation.m_ephemeris.begin(); it != m_constellation.m_ephemeris.end();) {
        qint64 diff = currentTime.secsTo(it.value().timestamp);
        if(abs(diff) > decayTime) {
            it = m_constellation.m_ephemeris.erase(it);
        }
        else {
            it++;
        }
    }
}

}// namespace WingletUI
