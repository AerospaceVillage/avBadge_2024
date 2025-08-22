#ifndef WINGLETUI_TIMEZONESETTINGMODEL_H
#define WINGLETUI_TIMEZONESETTINGMODEL_H

#include <QAbstractListModel>
#include <QList>

namespace WingletUI {

class TimeZoneOptionDir;

class TimeZoneSettingModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TimeZoneSettingModel(QObject *parent = nullptr);
    ~TimeZoneSettingModel();

    QModelIndex getIndexForPropVal(const QByteArray& tzId) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    static QString tzIdToPath(const QString &tzId);

private:
    TimeZoneOptionDir *rootOption;
};

} // namespace WingletUI

#endif // WINGLETUI_TIMEZONESETTINGMODEL_H
