#include "adsbreceiver.h"
#include "wingletgui.h"

namespace WingletUI {

ADSBReceiver::ADSBReceiver(QThread *ownerThread):
    AbstractSocketWorker(ownerThread)
{
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

} // namespace WingletUI
