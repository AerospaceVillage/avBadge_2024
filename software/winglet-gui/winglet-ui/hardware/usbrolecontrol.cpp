#include "usbrolecontrol.h"
#include "wingletgui.h"

#include <QFile>

namespace WingletUI {

USBRoleControl::USBRoleControl(QObject *parent)
    : QObject{parent}
{
    AppSettings *settings = &dynamic_cast<WingletGUI*>(parent)->settings;

    // Subscribe to events and set current role on startup
    connect(settings, SIGNAL(usbRoleChanged(int)), this, SLOT(roleChanged(int)));
    roleChanged(settings->usbRole());
}

void USBRoleControl::roleChanged(int new_role) {
    const char* roleStr;
    switch (new_role) {
    case USB_ROLE_HOST:
        roleStr = "host\n";
        break;
    case USB_ROLE_DEVICE:
        roleStr = "peripheral\n";
        break;
    case USB_ROLE_AUTO:
        roleStr = "otg\n";
        break;
    default:
        // Unknown setting
        return;
    }

    const char* filepath = "/sys/devices/platform/soc/4100000.usb/musb-hdrc.2.auto/mode";

    QFile f(filepath);
    if (!f.open(QFile::WriteOnly | QFile::Text)) {
        qWarning("USBRoleControl::roleChanged: Failed to open USB role file for writing: %s", filepath);
        return;
    }

    if (f.write(roleStr) < 0) {
        qWarning("USBRoleControl::roleChanged: Failed to write all bytes to role switch file");
    }
    f.close();
}

} // namespace WingletUI
