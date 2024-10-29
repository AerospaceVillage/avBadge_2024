#include "abstractsocketworker.h"

namespace WingletUI {

AbstractSocketWorker::AbstractSocketWorker(QThread *ownerThread)
{
    moveToThread(ownerThread);

    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(5000);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, SIGNAL(timeout()), this, SLOT(timeoutCallback()));

    reconnectTimer = new QTimer(this);
    reconnectTimer->setInterval(5000);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(attemptConnect()));

    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(socketDataReady()));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(stateChanged(QAbstractSocket::SocketState)));
}

AbstractSocketWorker::~AbstractSocketWorker()
{
    if (socket->state() != QAbstractSocket::UnconnectedState)
        socket->abort();
    delete socket;
    delete timeoutTimer;
    delete reconnectTimer;
}

void AbstractSocketWorker::start(const QString& hostName, quint16 port)
{
    if (started) {
        return;
    }

    this->hostName = hostName;
    this->port = port;
    started = true;
    attemptConnect();
}

void AbstractSocketWorker::attemptConnect()
{
    socket->connectToHost(hostName, port);
    timeoutTimer->start();
}

void AbstractSocketWorker::timeoutCallback()
{
    socket->abort();
}

void AbstractSocketWorker::socketConnected()
{
    timeoutTimer->stop();
    handleConnectionEvent(true);
    emit connectionStateChanged(true);
    isConnected = true;
}

void AbstractSocketWorker::writeData(const char *msg) {
     if (socket->state() == QAbstractSocket::ConnectedState)
         socket->write(msg, strlen(msg));
}

void AbstractSocketWorker::socketDataReady() {
    while (socket->canReadLine()) {
        QString line = socket->readLine();
        if (line.length() > 0)
            handleLine(line);
        else {
            qWarning("AbstractSocketWorker::socketDataReady: Failed to read line!");
            socket->abort();
        }
    }
}

void AbstractSocketWorker::stateChanged(QAbstractSocket::SocketState socketState)
{
    if (socketState == QAbstractSocket::UnconnectedState) {
        // Returned to unconnected state
        if (timeoutTimer->isActive()) {
            timeoutTimer->stop();
        }

        // If the connection was previously alive, notify of drop
        if (isConnected) {
            handleConnectionEvent(false);
            emit connectionStateChanged(false);
            isConnected = false;
        }

        // Retry connection after reconnect timeout
        reconnectTimer->start();
    }
}

} // namespace WingletUI
