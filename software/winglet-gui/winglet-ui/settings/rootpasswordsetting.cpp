#include "rootpasswordsetting.h"
#include "winglet-ui/windowcore/circularkeyboard.h"

#include <QProcess>

namespace WingletUI {

RootPasswordSetting::RootPasswordSetting(QObject *parent)
    : AbstractTextSetting(parent, "Set Root Password") {}

static bool mount_rw() {
    int ret = system("awk -F' ' '{if ($2 == \"/\") print $4;}' /proc/mounts | grep -q -E '(^|,)ro(,|$)'");
    bool needs_remount = (ret == 0);
    if (needs_remount) {
        ret = system("mount -o remount,rw /");
        if (ret) {
            qWarning("Remount rw returned non-zero code: %d", ret);
        }
    }
    return needs_remount;
}

static void mount_ro() {
    int ret = system("mount -o remount,ro /");
    if (ret) {
        qWarning("Remount ro returned non-zero code: %d", ret);
    }
}

void RootPasswordSetting::setValue(const QString &val) {
    bool needs_remount = mount_rw();

    // Send password change
    QProcess proc;
    proc.setStandardOutputFile(proc.nullDevice());
    proc.setStandardErrorFile(proc.nullDevice());
    proc.start("passwd", QStringList() << "-a" << "sha256" << "root");
    proc.waitForStarted(-1);
    QByteArray passwordEncoded = val.toLatin1();
    proc.write(passwordEncoded);
    proc.write("\n");
    proc.write(passwordEncoded);
    proc.write("\n");
    proc.closeWriteChannel();
    proc.waitForFinished(-1);
    int ret = proc.exitCode();
    if (ret != 0) {
        qWarning("Change password returned non-zero code: %d", ret);
    }

    if (needs_remount)
        mount_ro();
}

bool RootPasswordSetting::clearRootPassword() {
    bool needs_remount = mount_rw();

    // Clear password
    QProcess proc;
    proc.setStandardOutputFile(proc.nullDevice());
    proc.setStandardErrorFile(proc.nullDevice());
    proc.setStandardInputFile(proc.nullDevice());
    proc.start("passwd", QStringList() << "-d" << "root");
    proc.waitForFinished(-1);
    int ret = proc.exitCode();
    bool result = true;
    if (ret != 0) {
        qWarning("Clear root password returned non-zero code: %d", ret);
        result = false;
    }

    if (needs_remount)
        mount_ro();

    return result;
}

QList<QString> RootPasswordSetting::inputKeys() const {
    return CircularKeyboard::fullKeyboard;
}

};  // namespace WingletUI
