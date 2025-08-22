#ifndef WINGLETUI_WIFIMONITOR_H
#define WINGLETUI_WIFIMONITOR_H

#include <QTimer>
#include <QSocketNotifier>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>

struct wpa_ctrl;

namespace WingletUI {

struct WifiScanResult {
    QString ssid;
    int signalStrength;
    bool encrypted;
    bool known;
};

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
    void scanNetworks();
    QList<WifiScanResult> getScanResults();

signals:
    void wifiStateChanged(int state, int wifiStrength);
    void networkIdChanged(int networkId);
    void networkListChanged();
    void scanResultsChanged();

    // Private Signals
    void doAddOpenNetwork(QString ssid, QPrivateSignal);
    void doAddProtectedNetwork(QString ssid, QString psk, QPrivateSignal);
    void doRemoveNetwork(int networkId, QPrivateSignal);
    void doRaiseInterface(QPrivateSignal);
    void doLowerInterface(QPrivateSignal);
    void doScanNetworks(QPrivateSignal);
    void doRefreshScanResults(QPrivateSignal);

private slots:
    void pollTimerCallback();
    void asyncReadReady(QSocketDescriptor socket, QSocketNotifier::Type type);
    void addOpenNetworkInThread(QString ssid);
    void addProtectedNetworkInThread(QString ssid, QString psk);
    void removeNetworkInThread(int networkId);
    void raiseInterfaceInThread();
    void lowerInterfaceInThread();
    void scanNetworksInThread();
    void refreshScanResultsInThread();

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
    char* asyncRxBuf;
    const size_t rxBufLen = 4096;

    // State Reporting
    void reportNewWifiState(WifiState state, int strength);
    WifiState m_wifiState = WIFI_OFF;
    int m_wifiStrength = 0;
    int m_networkId = -1;

    QMap<QString, QString> *statusMap;
    QMap<int, QString> networkMap;

    // Scanning state
    bool m_scanInProgress = false;     // Holds if a scan is currently in progress to prevent multiple requests from going out
    bool m_scanResultsNeedsRedout = false;  // If true, then m_scanResults needs to be refreshed from the underlying wpa supplicant connection (starts as false since we're not connected)
    bool m_scanResetReadout = false;   // If true, then a scan finished as we were reading out the BSS data. The readout should be reset and started from the beginning (as the idx is no longer valid)
    bool m_scanReadoutInProgress = false;  // If true a scan readout is currently in progress (prevents reentry issues, maybe not needed but better to be safe)
    QMutex m_scanResultsMutex;
    QWaitCondition m_scanResultsReadyWait;
    QList<WifiScanResult> m_scanResults;
};

} // namespace WingletUI

Q_DECLARE_METATYPE(WingletUI::WifiScanResult);

#endif // WINGLETUI_WIFIMONITOR_H
