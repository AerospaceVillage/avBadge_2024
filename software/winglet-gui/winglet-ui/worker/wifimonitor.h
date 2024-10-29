#ifndef WINGLETUI_WIFIMONITOR_H
#define WINGLETUI_WIFIMONITOR_H

#include <QTimer>
#include <QSocketNotifier>
#include <QMap>

struct wpa_ctrl;

namespace WingletUI {

class WifiMonitor : public QObject
{
    Q_OBJECT

public:
    explicit WifiMonitor(QThread *ownerThread);
    ~WifiMonitor();

    enum WifiState {
        WIFI_OFF, WIFI_DISCONNECTED, WIFI_CONNECTING, WIFI_CONNECTED
    };

    WifiState wifiState() const { return m_wifiState; }
    int wifiStrength() const { return m_wifiStrength; }
    QMap<int, QString> knownNetworks() const { return networkMap; }
    int currentNetworkId() const { return m_networkId; }

    void addOpenNetwork(const QString &ssid);
    void addProtectedNetwork(const QString &ssid, const QString &psk);
    void removeNetwork(int networkId);
    void raiseInterface();
    void lowerInterface();

signals:
    void wifiStateChanged(int state, int wifiStrength);
    void networkIdChanged(int networkId);
    void networkListChanged();

    // Private Signals
    void doAddOpenNetwork(QString ssid, QPrivateSignal);
    void doAddProtectedNetwork(QString ssid, QString psk, QPrivateSignal);
    void doRemoveNetwork(int networkId, QPrivateSignal);
    void doRaiseInterface(QPrivateSignal);
    void doLowerInterface(QPrivateSignal);

private slots:
    void pollTimerCallback();
    void asyncReadReady(QSocketDescriptor socket, QSocketNotifier::Type type);
    void addOpenNetworkInThread(QString ssid);
    void addProtectedNetworkInThread(QString ssid, QString psk);
    void removeNetworkInThread(int networkId);
    void raiseInterfaceInThread();
    void lowerInterfaceInThread();

private:
    // Utility Functions
    bool tryConnectWpa();
    void disconnectWpa();
    bool refreshNetworkList();
    bool issueSimpleRequest(const QString &req);
    bool xferRequest(const char* req, bool ignoreMsgOverflow = false);
    bool decodePropertyString(const char* propertyStr, QMap<QString, QString> *mapOut);

    // WPA Supplicant Polling
    QTimer* pollTimer = NULL;
    QSocketNotifier* asyncMsgNotifier = NULL;

    // Underlying WPA Supplicant Storage
    struct wpa_ctrl* wpaCtrl = NULL;
    struct wpa_ctrl* wpaAsyncMsgCtrl = NULL;
    const int pollTimerRateMs = 1000;
    char* rxBuf;
    const size_t rxBufLen = 4096;

    // State Reporting
    void reportNewWifiState(WifiState state, int strength);
    WifiState m_wifiState = WIFI_OFF;
    int m_wifiStrength = 0;
    int m_networkId = -1;

    QMap<QString, QString> *statusMap;
    QMap<int, QString> networkMap;
};

} // namespace WingletUI

#endif // WINGLETUI_WIFIMONITOR_H
