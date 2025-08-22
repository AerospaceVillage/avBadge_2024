#ifndef WINGLETUI_WIFISCANMODEL_H
#define WINGLETUI_WIFISCANMODEL_H

#include <QAbstractItemModel>
#include <QPixmap>
#include "winglet-ui/worker/wifimonitor.h"

namespace WingletUI {

class WifiScanModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit WifiScanModel(QObject *parent = nullptr);

    QModelIndex getIndexForPropVal(const QByteArray& tzId) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private slots:
    void scanResultsChanged();
    void scanRequestTimerFired();
    void colorPaletteChanged();

private:
    void reloadPixmaps();

    QTimer m_scanRequestTimer;
    const int scanIntervalMs = 15000;
    QList<WifiScanResult> m_scanResults;

    QPixmap wifi_open_0bar;
    QPixmap wifi_open_1bar;
    QPixmap wifi_open_2bar;
    QPixmap wifi_open_3bar;
    QPixmap wifi_open_4bar;
    QPixmap wifi_psk_0bar;
    QPixmap wifi_psk_1bar;
    QPixmap wifi_psk_2bar;
    QPixmap wifi_psk_3bar;
    QPixmap wifi_psk_4bar;
};

} // namespace WingletUI

#endif // WINGLETUI_WIFISCANMODEL_H
