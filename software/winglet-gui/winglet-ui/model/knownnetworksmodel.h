#ifndef KNOWNNETWORKSMODEL_H
#define KNOWNNETWORKSMODEL_H

#include <QAbstractItemModel>
#include <QMap>

namespace WingletUI {

class KnownNetworksModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum KnownNetworkAction {
        INVALID_ACTION,
        FORGET_NETWORK
    };

    explicit KnownNetworksModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private slots:
    void networkIdChanged(int networkId);

private:
    QMap<int, QString> knownNetworks;
    QList<int> knownNetworkIdxMap;
    int currentNetworkId;
};

} // namespace WingletUI

#endif // KNOWNNETWORKSMODEL_H
