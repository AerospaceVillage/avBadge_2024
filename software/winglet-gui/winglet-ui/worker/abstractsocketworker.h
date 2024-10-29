#ifndef ABSTRACTSOCKETWORKER_H
#define ABSTRACTSOCKETWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace WingletUI {

class AbstractSocketWorker : public QObject
{
    Q_OBJECT
public:
    explicit AbstractSocketWorker(QThread *ownerThread);
    ~AbstractSocketWorker();

signals:
    void connectionStateChanged(bool connected);

protected:
    void start(const QString& hostName, quint16 port);
    void writeData(const char* msg);
    virtual void handleConnectionEvent(bool connected) { (void) connected; }
    virtual void handleLine(const QString &line) = 0;

private slots:
    void timeoutCallback();
    void attemptConnect();
    void socketConnected();
    void socketDataReady();

    void stateChanged(QAbstractSocket::SocketState socketState);

private:
    QString hostName;
    quint16 port = 0;
    bool started = false;
    bool isConnected = false;
    QTcpSocket *socket;
    QTimer *reconnectTimer;
    QTimer *timeoutTimer;
};

} // namespace WingletUI

#endif // ABSTRACTSOCKETWORKER_H
