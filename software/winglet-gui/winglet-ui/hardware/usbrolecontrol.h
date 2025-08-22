#ifndef WINGLETUI_USBROLECONTROL_H
#define WINGLETUI_USBROLECONTROL_H

#include <QObject>

namespace WingletUI {

class USBRoleControl : public QObject
{
    Q_OBJECT
public:
    explicit USBRoleControl(QObject *parent = nullptr);

protected slots:
    void roleChanged(int new_role);
};

} // namespace WingletUI

#endif // WINGLETUI_USBROLECONTROL_H
