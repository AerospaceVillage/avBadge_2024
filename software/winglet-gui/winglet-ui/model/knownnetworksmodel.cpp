#include "knownnetworksmodel.h"
#include "wingletgui.h"

namespace WingletUI {

const QList<QPair<QString, int>> networkOptions = {
    {"Forget Nework", KnownNetworksModel::FORGET_NETWORK}
};

const quintptr rootRowInternalId = 0xFFFFFFFF;

KnownNetworksModel::KnownNetworksModel(QObject *parent)
    : QAbstractItemModel{parent}
{
    auto wifiMon = WingletGUI::inst->wifiMon;

    knownNetworks = wifiMon->knownNetworks();

    // Create map from row to known network index
    knownNetworkIdxMap.reserve(knownNetworks.size());
    for (auto itr = knownNetworks.begin(); itr != knownNetworks.end(); itr++) {
        knownNetworkIdxMap.append(itr.key());
    }

    // Get the current network ID and subscribe to changes so we can refresh the UI
    connect(wifiMon, SIGNAL(networkIdChanged(int)), this, SLOT(networkIdChanged(int)));
    currentNetworkId = wifiMon->currentNetworkId();
}

void KnownNetworksModel::networkIdChanged(int networkId) {
    int oldNetworkId = currentNetworkId;
    currentNetworkId = networkId;

    int oldIdx = knownNetworkIdxMap.indexOf(oldNetworkId);
    int newIdx = knownNetworkIdxMap.indexOf(currentNetworkId);

    if (oldIdx < 0 || newIdx < 0 || oldIdx != newIdx) {
        return;
    }

    // Report that the current network label changed
    if (oldIdx < newIdx) {
        emit dataChanged(index(oldIdx, 0), index(newIdx, 0), {Qt::EditRole});
    }
    else {
        emit dataChanged(index(newIdx, 0), index(oldIdx, 0), {Qt::EditRole});
    }
}

QVariant KnownNetworksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        if (role == Qt::DisplayRole) {
            return "Known Networks";
        }
        else {
            return {};
        }
    }

    if (index.internalId() == rootRowInternalId) {
        // This is the root row (list of networks)
        if (index.row() < 0 || index.row() >= knownNetworkIdxMap.size() || index.column() != 0) {
            return {};
        }

        int networkId = knownNetworkIdxMap.at(index.row());

        if (role == Qt::DisplayRole) {
            auto networkName = knownNetworks.constFind(networkId);
            if (networkName == knownNetworks.end()) {
                return {};
            }
            else {
                return networkName.value();
            }
        }
        else if (role == Qt::EditRole) {
            if (networkId == currentNetworkId) {
                return "Connected";
            }
            else {
                return {};
            }
        }
        else {
            return {};
        }
    }
    else {
        // This is a row for the list of options
        if (index.row() < 0 || index.row() >= networkOptions.size() || index.column() != 0) {
            return {};
        }

        if (role == Qt::DisplayRole) {
            return networkOptions.at(index.row()).first;
        }
        else if (role == Qt::UserRole) {
            return networkOptions.at(index.row()).second;
        }
        else {
            return {};
        }
    }
}

QModelIndex KnownNetworksModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Make sure parent is valid
        if (parent.row() < 0 || parent.row() >= knownNetworkIdxMap.size() || parent.column() != 0 || parent.internalId() != rootRowInternalId) {
            return {};
        }

        int networkId = knownNetworkIdxMap.at(parent.row());
        return createIndex(row, column, networkId);
    }
    else {
        return createIndex(row, column, rootRowInternalId);
    }
}

QModelIndex KnownNetworksModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.internalId() == rootRowInternalId) {
        return {};
    }
    else {
        return this->index(index.internalId(), 0, {});
    }
}

int KnownNetworksModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return knownNetworkIdxMap.size();
    }
    else if (parent.internalId() == rootRowInternalId && parent.row() >= 0 && parent.row() < knownNetworkIdxMap.size()) {
        return networkOptions.size();
    }
    else {
        return 0;
    }
}

int KnownNetworksModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid() || parent.internalId() == rootRowInternalId) {
        return 1;
    }
    else {
        return 0;
    }
}

} // namespace WingletUI
