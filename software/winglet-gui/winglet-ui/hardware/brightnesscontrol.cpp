#include "brightnesscontrol.h"
#include "wingletgui.h"

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define BRIGHTNESS_FILE "/sys/class/backlight/panel-backlight/brightness"

namespace WingletUI {

BrightnessControl::BrightnessControl(QObject *parent)
    : QObject{parent}
{
    brightnessFd = open(BRIGHTNESS_FILE, O_WRONLY);
    if (brightnessFd < 0) {
        qWarning("BrightnessControl: Failed to open sysfs file '%s' (%d)", BRIGHTNESS_FILE, errno);
    }

    AppSettings *settings = &dynamic_cast<WingletGUI*>(parent)->settings;

    connect(settings, SIGNAL(screenBrightnessChanged(int)), this, SLOT(brightnessChanged(int)));
    brightnessChanged(settings->screenBrightness());
}

BrightnessControl::~BrightnessControl()
{
    if (brightnessFd >= 0)
        close(brightnessFd);
}

void BrightnessControl::brightnessChanged(int value) {
    if (brightnessFd < 0) {
        return;
    }

    QString msg = QString("%1\n").arg(value);
    QByteArray byteEncoding = msg.toLatin1();
    int ret = write(brightnessFd, byteEncoding.data(), byteEncoding.length());
    if (ret < 0) {
        qWarning("BrightnessControl::brightnessChanged: Failed to write (%d)", errno);
    }
    else if (ret != byteEncoding.length()) {
        qWarning("BrightnessControl::brightnessChanged: Not all bytes written (only wrote %d out of %d bytes)", ret, byteEncoding.length());
    }
}

} // namespace WingletUI
