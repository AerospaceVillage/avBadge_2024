#ifndef WINGLETUI_SETTINGSMENUMODEL_H
#define WINGLETUI_SETTINGSMENUMODEL_H

#include <QAbstractItemModel>
#include "winglet-ui/settings/abstractsettingsentry.h"

namespace WingletUI {

class SettingsMenuModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SettingsMenuModel(const SettingsEntryContainer* menuRoot, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private slots:
    void settingsEntryChanged(QObject *obj);

private:
    QModelIndex getObjectIndex(AbstractSettingsEntry *entry) const;
    const SettingsEntryContainer* menuRoot;
};

} // namespace WingletUI

#endif // WINGLETUI_SETTINGSMENUMODEL_H
