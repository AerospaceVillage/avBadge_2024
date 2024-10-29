#ifndef WINGLETUI_APPSETTINGSENUMSELECTIONMODEL_H
#define WINGLETUI_APPSETTINGSENUMSELECTIONMODEL_H

#include <QAbstractListModel>

namespace WingletUI {

class AppSettingsEnumSelectionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AppSettingsEnumSelectionModel(QObject *parent, QString title,
                                           const QVector<QPair<int, QString>>& valueMap):
        QAbstractListModel{parent}, title(title), valueLookup(valueMap) {}

    QModelIndex getIndexForPropVal(int val) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void setSelectedValue(QModelIndex idx);

private:
    QString title;
    QVector<QPair<int, QString>> valueLookup;
    int selectedValueIdx;
};

} // namespace WingletUI

#endif // WINGLETUI_APPSETTINGSENUMSELECTIONMODEL_H
