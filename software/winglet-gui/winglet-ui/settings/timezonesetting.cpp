#include "timezonesetting.h"
#include <QTimeZone>
#include <QFile>

#include "wingletgui.h"

namespace WingletUI {

const static QString localtimeFilePath = "/var/localtime";
const static QString timezoneFilePath = "/var/timezone";

TimeZoneSetting::TimeZoneSetting(QObject *parent):
    AbstractListSetting(parent, "Timezone", {}),
    model(new TimeZoneSettingModel(this)) {}

QString TimeZoneSetting::curValueName() const
{
    return QTimeZone::systemTimeZoneId();
}

QModelIndex TimeZoneSetting::curIndex() const
{
    return model->getIndexForPropVal(QTimeZone::systemTimeZoneId());
}

void TimeZoneSetting::setValue(QModelIndex idx)
{
    auto tzIdVariant = idx.data(Qt::UserRole);
    if (!tzIdVariant.isNull()) {
        QByteArray tzId = tzIdVariant.toByteArray();
        if (tzId == QTimeZone::systemTimeZoneId()) {
            // No need to set the timezone if it's already set
            return;
        }

        QString timezonePath = TimeZoneSettingModel::tzIdToPath(tzId);

        // Set the localtime symlink
        QFile::remove(localtimeFilePath);
        if (!QFile::link(timezonePath, localtimeFilePath)) {
            qWarning("Failed to create symlink for localtime");
            return;
        }

        // Write the timezone file as well
        QFile timezoneFile(timezoneFilePath);
        if (!timezoneFile.open(QFile::WriteOnly | QFile::Truncate)) {
            qWarning("Failed to open timezone file for writing");
            return;
        }

        timezoneFile.write(tzId);
        timezoneFile.write("\n");
        timezoneFile.close();

        // Timezone successfully written, set to mark that a restart is required
        WingletGUI::inst->settings.rebootNeeded = true;
    }
}

} // namespace WingletUI
