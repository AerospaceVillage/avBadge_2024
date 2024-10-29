#ifndef WINGLETUI_TIMEZONESETTING_H
#define WINGLETUI_TIMEZONESETTING_H

#include "winglet-ui/settings/abstractsettingsentry.h"
#include "winglet-ui/model/timezonesettingmodel.h"

namespace WingletUI {

class TimeZoneSetting : public AbstractListSetting
{
    Q_OBJECT
public:
    TimeZoneSetting(QObject *parent);

    QAbstractItemModel* selectionModel() override { return model; }
    QString curValueName() const override;
    QModelIndex curIndex() const override;
    void setValue(QModelIndex idx) override;

private:
    TimeZoneSettingModel *model;
    bool timezoneSet = false;
};

} // namespace WingletUI

#endif // WINGLETUI_TIMEZONESETTING_H
