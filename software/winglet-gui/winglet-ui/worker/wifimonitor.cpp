#include "wifimonitor.h"

#include <QTextStream>
#include <QThread>

#ifndef NO_HARDWARE
#include <wpa_ctrl.h>
#else
static inline __attribute__((unused)) struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path) {
    (void) ctrl_path;
    return NULL;
}
static inline __attribute__((unused)) struct wpa_ctrl * wpa_ctrl_open2(const char *ctrl_path, const char *cli_path) {
    (void) ctrl_path;
    (void) cli_path;
    return NULL;
}
static inline __attribute__((unused)) void wpa_ctrl_close(struct wpa_ctrl *ctrl) {
    (void) ctrl;
}
static inline __attribute__((unused)) int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
             char *reply, size_t *reply_len,
                                   void (*msg_cb)(char *msg, size_t len)) {
    (void) ctrl;
    (void) cmd;
    (void) cmd_len;
    (void) reply;
    (void) reply_len;
    (void) msg_cb;
    return -1;
}
static inline __attribute__((unused)) int wpa_ctrl_attach(struct wpa_ctrl *ctrl) {
    (void) ctrl;
    return -1;
}
static inline __attribute__((unused)) int wpa_ctrl_detach(struct wpa_ctrl *ctrl) {
    (void) ctrl;
    return -1;
}
static inline __attribute__((unused)) int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len) {
    (void) ctrl;
    (void) reply;
    (void) reply_len;
    return -1;
}
static inline __attribute__((unused)) int wpa_ctrl_pending(struct wpa_ctrl *ctrl) {
    (void) ctrl;
    return -1;
}
static inline __attribute__((unused)) int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl) {
    (void) ctrl;
    return -1;
}
#endif

namespace WingletUI {

#define WPA_INTERFACE_PATH "/var/run/wpa_supplicant/wlan0"

WifiMonitor::WifiMonitor(QThread *ownerThread)
{
    moveToThread(ownerThread);

    rxBuf = new char[rxBufLen];
    statusMap = new QMap<QString, QString>();

    connect(this, SIGNAL(doAddOpenNetwork(QString)), this, SLOT(addOpenNetworkInThread(QString)));
    connect(this, SIGNAL(doAddProtectedNetwork(QString, QString)), this, SLOT(addProtectedNetworkInThread(QString, QString)));
    connect(this, SIGNAL(doRemoveNetwork(int)), this, SLOT(removeNetworkInThread(int)));
    connect(this, SIGNAL(doRaiseInterface()), this, SLOT(raiseInterfaceInThread()));
    connect(this, SIGNAL(doLowerInterface()), this, SLOT(lowerInterfaceInThread()));

    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollTimerCallback()));
    pollTimer->start(pollTimerRateMs);
    pollTimerCallback();
}

WifiMonitor::~WifiMonitor()
{
    if (wpaCtrl) {
        wpa_ctrl_close(wpaCtrl);
    }
    if (asyncMsgNotifier) {
        delete asyncMsgNotifier;
    }
    if (wpaAsyncMsgCtrl) {
        wpa_ctrl_close(wpaAsyncMsgCtrl);
    }
    delete pollTimer;
    delete[] rxBuf;
    delete statusMap;
}

void WifiMonitor::reportNewWifiState(WifiState state, int strength) {
    if (state == m_wifiState && strength == m_wifiStrength) {
        return;
    }

    m_wifiState = state;
    m_wifiStrength = strength;
    emit wifiStateChanged(state, strength);
}

void WifiMonitor::asyncReadReady(QSocketDescriptor socket, QSocketNotifier::Type type)
{
    (void) socket;
    if (type != QSocketNotifier::Read) {
        // We've only subscribed to read events
        return;
    }

    int ret;
    while ((ret = wpa_ctrl_pending(wpaAsyncMsgCtrl)) == 1) {
        size_t rxLen = rxBufLen;
        ret = wpa_ctrl_recv(wpaAsyncMsgCtrl, rxBuf, &rxLen);
        if (ret) {
            disconnectWpa();
            return;
        }

        // Process the message
        if (rxLen < rxBufLen) {
            rxBuf[rxLen] = 0;
        }
        else {
            qWarning("WifiMonitor::asyncReadReady: Buffer truncated - packet too large");
            disconnectWpa();
            return;
        }

        QString msg(rxBuf);
        msg = msg.trimmed();
        if (msg.length() > 0) {
            // Truncate prefix log level (we don't care about it, just the message)
            if (msg.at(0) == '<') {
                int prefixEndIdx = msg.indexOf('>');
                if (prefixEndIdx > 0) {
                    msg = msg.right(msg.size() - prefixEndIdx - 1);
                }
            }

            // Refresh network list if we get an event that a network was added or removed (both by us or external programs)
            if (msg.startsWith("CTRL-EVENT-NETWORK-REMOVED") || msg.startsWith("CTRL-EVENT-NETWORK-ADDED")) {
                if (!refreshNetworkList()) {
                    disconnectWpa();
                }
            }
        }
    }
    if (ret < 0) {
        disconnectWpa();
    }
}

bool WifiMonitor::tryConnectWpa()
{
    if (wpaCtrl != NULL) {
        // Already connected
        return true;
    }

    wpaCtrl = wpa_ctrl_open(WPA_INTERFACE_PATH);
    if (!wpaCtrl) {
         goto error_open_comm;
    }

    wpaAsyncMsgCtrl = wpa_ctrl_open(WPA_INTERFACE_PATH);
    if (!wpaAsyncMsgCtrl) {
        goto error_open_async;
    }

    if (wpa_ctrl_attach(wpaAsyncMsgCtrl)) {
        goto error_attach_async;
    }

    asyncMsgNotifier = new QSocketNotifier(wpa_ctrl_get_fd(wpaAsyncMsgCtrl), QSocketNotifier::Read, this);
    connect(asyncMsgNotifier, SIGNAL(activated(QSocketDescriptor, QSocketNotifier::Type)), this, SLOT(asyncReadReady(QSocketDescriptor, QSocketNotifier::Type)));

    if (!refreshNetworkList()) {
        goto error_network_rescan;
    }

    return true;

error_network_rescan:
    asyncMsgNotifier->setEnabled(false);
    asyncMsgNotifier->deleteLater();
    asyncMsgNotifier = NULL;
error_attach_async:
    wpa_ctrl_close(wpaAsyncMsgCtrl);
    wpaAsyncMsgCtrl = NULL;
error_open_async:
    wpa_ctrl_close(wpaCtrl);
    wpaCtrl = NULL;
error_open_comm:
    return false;
}

void WifiMonitor::disconnectWpa()
{
    if (asyncMsgNotifier) {
        // Can't delete right away, since we may be in a callback for a signal for this very notifier
        asyncMsgNotifier->setEnabled(false);
        asyncMsgNotifier->deleteLater();
        asyncMsgNotifier = NULL;
    }
    if (wpaCtrl) {
        wpa_ctrl_close(wpaCtrl);
        wpaCtrl = NULL;
    }
    if (wpaAsyncMsgCtrl) {
        wpa_ctrl_close(wpaAsyncMsgCtrl);
        wpaAsyncMsgCtrl = NULL;
    }

    networkMap.clear();
    bool needsNetworkIdEmit = (m_networkId != -1);
    m_networkId = -1;

    reportNewWifiState(WIFI_OFF, -1);
    if (needsNetworkIdEmit)
        emit networkIdChanged(m_networkId);
    emit networkListChanged();
}

bool WifiMonitor::xferRequest(const char* req, bool ignoreMsgOverflow) {
    if (!wpaCtrl)
        return false;

    size_t rxLen = rxBufLen;
    if (ignoreMsgOverflow)
        rxLen--;

    int ret = wpa_ctrl_request(wpaCtrl, req, strlen(req), rxBuf, &rxLen, NULL);
    if (ret) {
        return false;
    }
    if (!ignoreMsgOverflow && rxLen >= rxBufLen) {
        qWarning("WifiMonitor::xferRequest: Error - response packet too large");
        return false;
    }

    if (rxLen >= rxBufLen)
        rxBuf[rxBufLen - 1] = 0;
    else
        rxBuf[rxLen] = 0;
    return true;
}

bool WifiMonitor::decodePropertyString(const char* propertyStr, QMap<QString, QString> *mapOut)
{
    QString line;
    QTextStream stream(propertyStr);

    bool foundOne = false;
    while (stream.readLineInto(&line)) {
        int idx = line.indexOf('=');
        if (idx <= 0) {
            // Line doesn't contain equals or has empty key, not valid
            return false;
        }
        mapOut->insert(line.mid(0, idx), line.mid(idx+1));
        foundOne = true;
    }

    return foundOne;
}

void WifiMonitor::raiseInterface() {
    emit doRaiseInterface(QPrivateSignal());
}

void WifiMonitor::lowerInterface() {
    emit doLowerInterface(QPrivateSignal());
}

void WifiMonitor::raiseInterfaceInThread() {
    (void)(system("ifup wlan0 >/dev/null 2>&1")+1);
}

void WifiMonitor::lowerInterfaceInThread() {
    disconnectWpa();
    (void)(system("ifdown wlan0 >/dev/null 2>&1")+1);
}

void WifiMonitor::addOpenNetwork(const QString &ssid) {
    emit doAddOpenNetwork(ssid, QPrivateSignal());
}

void WifiMonitor::addProtectedNetwork(const QString &ssid, const QString &psk) {
    emit doAddProtectedNetwork(ssid, psk, QPrivateSignal());
}

void WifiMonitor::removeNetwork(int networkId) {
    emit doRemoveNetwork(networkId, QPrivateSignal());
}

void WifiMonitor::addOpenNetworkInThread(QString ssid) {
    if (!xferRequest("ADD_NETWORK")) {
        qWarning("Failed to send add network command");
        return;
    }

    bool networkIdParseOkay = false;
    int newNetworkId = QString(rxBuf).trimmed().toInt(&networkIdParseOkay);
    if (!networkIdParseOkay) {
        qWarning("Failed to add network: invalid id returned '%s'", rxBuf);
        return;
    }

    QString ssidEscaped = ssid.replace("\\", "\\\\").replace("\"", "\\\"");
    if (!issueSimpleRequest(QString("SET_NETWORK %1 ssid \"%2\"").arg(newNetworkId).arg(ssidEscaped)))
        return;

    if (!issueSimpleRequest(QString("SET_NETWORK %1 key_mgmt NONE").arg(newNetworkId)))
        return;

    if (!issueSimpleRequest(QString("ENABLE_NETWORK %1").arg(newNetworkId)))
        return;

    if (!issueSimpleRequest("SAVE_CONFIG"))
        return;
}

void WifiMonitor::addProtectedNetworkInThread(QString ssid, QString psk) {
    if (!xferRequest("ADD_NETWORK")) {
        qWarning("Failed to send add network command");
        return;
    }

    bool networkIdParseOkay = false;
    int newNetworkId = QString(rxBuf).trimmed().toInt(&networkIdParseOkay);
    if (!networkIdParseOkay) {
        qWarning("Failed to add network: invalid id returned '%s'", rxBuf);
        return;
    }

    QString ssidEscaped = ssid.replace("\"", "\\\"");
    if (!issueSimpleRequest(QString("SET_NETWORK %1 ssid \"%2\"").arg(newNetworkId).arg(ssidEscaped)))
        return;

    QString pskEscaped = psk.replace("\\", "\\\\").replace("\"", "\\\"");
    if (!issueSimpleRequest(QString("SET_NETWORK %1 psk \"%2\"").arg(newNetworkId).arg(pskEscaped)))
        return;

    if (!issueSimpleRequest(QString("ENABLE_NETWORK %1").arg(newNetworkId)))
        return;

    if (!issueSimpleRequest("SAVE_CONFIG"))
        return;
}

void WifiMonitor::removeNetworkInThread(int networkId) {
    if (!issueSimpleRequest(QString("REMOVE_NETWORK %1").arg(networkId)))
        return;
    issueSimpleRequest("SAVE_CONFIG");
}

bool WifiMonitor::issueSimpleRequest(const QString &req) {
    const char* reqCstr = req.toLatin1().data();
    if (!xferRequest(reqCstr)) {
        qWarning("Failed to send '%s'", reqCstr);
        return false;
    }

    if (QString(rxBuf).trimmed() != "OK") {
        qWarning("Failed to execute '%s': wpa_supplicant returned '%s'", reqCstr, rxBuf);
        return false;
    }

    return true;
}

bool WifiMonitor::refreshNetworkList() {
    char *start, *end, *id, *ssid, *bssid, *flags;

    networkMap.clear();

    if (!xferRequest("LIST_NETWORKS", true))
        return false;

    start = strchr(rxBuf, '\n');
    if (start == NULL)
        return false;
    start++;

    while (*start) {
        bool last = false;
        end = strchr(start, '\n');
        if (end == NULL) {
            last = true;
            end = start;
            while (end[0] && end[1])
                end++;
        }
        *end = '\0';

        id = start;
        ssid = strchr(id, '\t');
        if (ssid == NULL)
            break;
        *ssid++ = '\0';
        bssid = strchr(ssid, '\t');
        if (bssid == NULL)
            break;
        *bssid++ = '\0';
        flags = strchr(bssid, '\t');
        if (flags == NULL)
            break;
        *flags++ = '\0';

        if (strstr(flags, "[DISABLED][P2P-PERSISTENT]")) {
            if (last)
                break;
            start = end + 1;
            continue;
        }

        bool idConvOkay;
        int idVal = QString(id).toUInt(&idConvOkay);
        if (!idConvOkay) {
            continue;
        }
        networkMap.insert(idVal, ssid);

        start = end + 1;
        if (*start && strchr(start, '\n'))
            continue;

        /* avoid race conditions */
        QThread::msleep(200);
        QString cmd("LIST_NETWORKS LAST_ID=");
        cmd.append(id);
        if (!xferRequest(cmd.toLatin1().data(), true))
            break;

        start = strchr(rxBuf, '\n');
        if (!start)
            break;
        start++;
    }

    emit networkListChanged();

    return true;
}

void WifiMonitor::pollTimerCallback()
{
    bool needsNetIdEmit = false;

    if (tryConnectWpa()) {
        QMap<QString, QString>::const_iterator itr;
        WifiState state;
        int signalLevel = -1;

        if (!xferRequest("STATUS")) {
            goto comm_fail;
        }

        statusMap->clear();
        if (!decodePropertyString(rxBuf, statusMap)) {
            goto comm_fail;
        }

        itr = statusMap->constFind("wpa_state");
        if (itr == statusMap->end()) {
            // Needs to have wpa_state
            goto comm_fail;
        }

        if (*itr == "SCANNING" || *itr == "DISCONNECTED" || *itr == "INACTIVE" || *itr == "INTERFACE_DISABLED") {
            state = WIFI_DISCONNECTED;
        }
        else if (*itr == "COMPLETED") {
            if (statusMap->contains("ip_address")) {
                // Only show connected if we got an IP address
                state = WIFI_CONNECTED;
            }
            else {
                state = WIFI_CONNECTING;
            }
        }
        else {
            // Fall back to all other states connecting
            state = WIFI_CONNECTING;
        }

        // Grab the currently connected network id
        itr = statusMap->constFind("id");
        if (itr != statusMap->end()) {
            bool okay;
            int id = itr->toInt(&okay);
            if (okay) {
                needsNetIdEmit = (id != m_networkId);
                m_networkId = id;
            }
            else {
                needsNetIdEmit = (m_networkId != -1);
                m_networkId = -1;
            }
        }
        else {
            needsNetIdEmit = (m_networkId != -1);
            m_networkId = -1;
        }
        if (needsNetIdEmit)
            emit networkIdChanged(m_networkId);

        if (state == WIFI_CONNECTING || state == WIFI_CONNECTED) {
            if (!xferRequest("SIGNAL_POLL")) {
                goto comm_fail;
            }

            // Try to decode signal strength (but don't fail if decode fails, we may be in a weird connection state)
            if (decodePropertyString(rxBuf, statusMap)) {
                bool okay = false;
                itr = statusMap->constFind("AVG_RSSI");
                if (itr != statusMap->end()) {
                    signalLevel = itr->toInt(&okay);
                    if (okay) {
                        signalLevel = 2 * (100 + signalLevel);
                        if (signalLevel > 100)
                            signalLevel = 100;
                        if (signalLevel < 0)
                            signalLevel = 0;
                    }
                    else {
                        signalLevel = -1;
                    }
                }
            }
        }

        reportNewWifiState(state, signalLevel);
        return;

    comm_fail:
        disconnectWpa();
        return;
    }
}

} // namespace WingletUI
