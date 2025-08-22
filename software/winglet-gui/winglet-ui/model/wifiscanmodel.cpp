#include "wifiscanmodel.h"
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include <QPalette>

namespace WingletUI {

WifiScanModel::WifiScanModel(QObject *parent)
    : QAbstractItemModel{parent}, m_scanRequestTimer(this)
{
    // Load all icons (coloring according to color palette)
    reloadPixmaps();

    // Subscribe to color palette change events so the bg logo is updated if dark mode is changed
    connect(activeTheme, SIGNAL(colorPaletteChanged()), this, SLOT(colorPaletteChanged()));

    connect(WingletGUI::inst->wifiMon, SIGNAL(scanResultsChanged()), this, SLOT(scanResultsChanged()));
    connect(&m_scanRequestTimer, SIGNAL(timeout()), this, SLOT(scanRequestTimerFired()));
    m_scanRequestTimer.setInterval(scanIntervalMs);

    // Fetch the scan results
    WingletGUI::inst->wifiMon->scanNetworks();
    m_scanResults = WingletGUI::inst->wifiMon->getScanResults();
    m_scanRequestTimer.start();
}

void WifiScanModel::reloadPixmaps() {
    activeTheme->loadMonochromeIcon(&wifi_open_0bar, ":/icons/settings/wifi_open_0bar.png");
    activeTheme->loadMonochromeIcon(&wifi_open_1bar, ":/icons/settings/wifi_open_1bar.png");
    activeTheme->loadMonochromeIcon(&wifi_open_2bar, ":/icons/settings/wifi_open_2bar.png");
    activeTheme->loadMonochromeIcon(&wifi_open_3bar, ":/icons/settings/wifi_open_3bar.png");
    activeTheme->loadMonochromeIcon(&wifi_open_4bar, ":/icons/settings/wifi_open_4bar.png");
    activeTheme->loadMonochromeIcon(&wifi_psk_0bar, ":/icons/settings/wifi_psk_0bar.png");
    activeTheme->loadMonochromeIcon(&wifi_psk_1bar, ":/icons/settings/wifi_psk_1bar.png");
    activeTheme->loadMonochromeIcon(&wifi_psk_2bar, ":/icons/settings/wifi_psk_2bar.png");
    activeTheme->loadMonochromeIcon(&wifi_psk_3bar, ":/icons/settings/wifi_psk_3bar.png");
    activeTheme->loadMonochromeIcon(&wifi_psk_4bar, ":/icons/settings/wifi_psk_4bar.png");
}

void WifiScanModel::colorPaletteChanged() {
    reloadPixmaps();

    QVector<int> rolesChanged;
    rolesChanged.push_back(Qt::DecorationRole);
    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), 0), rolesChanged);
}

void WifiScanModel::scanRequestTimerFired() {
    WingletGUI::inst->wifiMon->scanNetworks();
    m_scanRequestTimer.start();
}

void WifiScanModel::scanResultsChanged() {

    // 1. Prepare for scan results refresh
    emit layoutAboutToBeChanged();

    // 2. Save previous state so we can update the persistent indices
    QModelIndexList oldPersistentIndices = persistentIndexList();
    QList<WifiScanResult> oldScanResults = m_scanResults;

    // 3. Refresh scan results
    m_scanResults = WingletGUI::inst->wifiMon->getScanResults();

    // 4. Update the persistent model indices
    if (oldPersistentIndices.size() > 2) {
        // For a lot of indices, save a map to avoid costly searches
        QMap<QString, int> ssidMap;
        for (int i = 0; i < m_scanResults.count(); i++) {
            ssidMap.insert(m_scanResults.at(i).ssid, i);
        }

        for (auto oldIdx : oldPersistentIndices) {
            if (!oldIdx.isValid())
                continue;

            QModelIndex newIdx;
            // Make sure the index points to a valid item before trying to decode it
            if (!oldIdx.parent().isValid() && oldIdx.column() == 0 && oldIdx.row() >= 0 && oldIdx.row() < oldScanResults.count()) {
                const WifiScanResult& oldResult = oldScanResults.at(oldIdx.row());
                auto it = ssidMap.constFind(oldResult.ssid);
                if (it != ssidMap.cend()) {
                    // Found the SSID in the new list
                    newIdx = createIndex(it.value(), 0);
                }
            }
            changePersistentIndex(oldIdx, newIdx);
        }
    }
    else if (oldPersistentIndices.size() > 0) {
        // For only a small nubmer of persistent indices, search one by one without caching into map
        for (auto oldIdx : oldPersistentIndices) {
            if (!oldIdx.isValid())
                continue;

            QModelIndex newIdx;
            // Make sure the index points to a valid item before trying to decode it
            if (!oldIdx.parent().isValid() && oldIdx.column() == 0 && oldIdx.row() >= 0 && oldIdx.row() < oldScanResults.count()) {
                const WifiScanResult& oldResult = oldScanResults.at(oldIdx.row());

                for (int i = 0; i < m_scanResults.count(); i++) {
                    if (m_scanResults.at(i).ssid == oldResult.ssid) {
                        // Found the SSID in the new list
                        newIdx = createIndex(i, 0);
                    }
                }
            }
            changePersistentIndex(oldIdx, newIdx);
        }
    }

    // 5. Notify of layout changed
    emit layoutChanged();

    // Reset scan timer
    m_scanRequestTimer.start();
}

QVariant WifiScanModel::data(const QModelIndex &index, int role) const
{
    // Invalid index is the root node
    if (!index.isValid()) {
        if (role == Qt::DisplayRole) {
            // Set the title for the menu
            return "WiFi\nScan";
        }
        else {
            return {};
        }
    }

    // If no scan results right now, report scanning
    if (m_scanResults.empty()) {
        if (role == Qt::DisplayRole)
            return "Scanning...";
        else
            return {};
    }

    // We have scan results, return the field
    int idx = index.row();
    if (idx >= m_scanResults.count() || idx < 0)
        return {};

    const WifiScanResult &result = m_scanResults.at(idx);

    if (role == Qt::DisplayRole) {
        return result.ssid;
    }
    else if (role == Qt::DecorationRole) {
        if (result.encrypted) {
            if (result.signalStrength >= 80)
                return wifi_psk_4bar;
            else if (result.signalStrength >= 60)
                return wifi_psk_3bar;
            else if (result.signalStrength >= 40)
                return wifi_psk_2bar;
            else if (result.signalStrength >= 20)
                return wifi_psk_1bar;
            else
                return wifi_psk_0bar;
        }
        else {
            if (result.signalStrength >= 80)
                return wifi_open_4bar;
            else if (result.signalStrength >= 60)
                return wifi_open_3bar;
            else if (result.signalStrength >= 40)
                return wifi_open_2bar;
            else if (result.signalStrength >= 20)
                return wifi_open_1bar;
            else
                return wifi_open_0bar;
        }
    }
    else if (role == Qt::UserRole) {
        return QVariant::fromValue(result);
    }
    else if (role == Qt::EditRole) {
        if (result.known) {
            QString curSsid = WingletGUI::inst->wifiMon->knownNetworks().value(WingletGUI::inst->wifiMon->currentNetworkId(), "");
            return curSsid == result.ssid ? "Connected" : "Saved";
        }
        else {
            return {};
        }
    }
    else {
        return {};
    }
}

QModelIndex WifiScanModel::index(int row, int column, const QModelIndex &parent) const
{
    // Only column 0 allowed
    if (column != 0)
        return {};

    // Can't access children elements
    if (parent.isValid())
        return {};

    if (m_scanResults.empty()) {
        // No Scan Results
        if (row == 0)
            return createIndex(0, 0);  // Return index for first element to show a Scanning... text
        else
            return {};
    }
    else {
        // Normal Behavior (We have scan results)
        // Ensure index is in bounds
        if (row < 0 || row >= m_scanResults.count())
            return {};

        return createIndex(row, 0);
    }
}

QModelIndex WifiScanModel::parent(const QModelIndex &index) const {
    (void) index;
    // Flat list (no parents)
    return {};
}

Qt::ItemFlags WifiScanModel::flags(const QModelIndex &index) const {
    (void) index;
    if (m_scanResults.empty())
        return Qt::NoItemFlags;  // If empty disable selecting the Scanning item
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int WifiScanModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;  // Flat List (no children allowed)
    }

    if (m_scanResults.empty())
        return 1;  // 1 item for the Scanning... text
    else
        return m_scanResults.count();
}

int WifiScanModel::columnCount(const QModelIndex &parent) const
{
    (void) parent;
    return 1;
}

} // namespace WingletUI
