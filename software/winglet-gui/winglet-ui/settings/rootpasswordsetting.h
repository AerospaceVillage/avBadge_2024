#ifndef ROOTPASSWORDSETTING_H
#define ROOTPASSWORDSETTING_H

#include "abstractsettingsentry.h"

namespace WingletUI {

class RootPasswordSetting : public AbstractTextSetting
{
    Q_OBJECT
public:
    explicit RootPasswordSetting(QObject *parent);

    static bool clearRootPassword();

    QString value() const override { return ""; }  // We don't have a password to show
    void setValue(const QString &val) override;
    QList<QString> inputKeys() const override;

    QString prompt() const override { return "This allows SSH login over WiFi\nEnter New Password:"; }
    QString title() const override { return "Set Root Password"; }
    int maxLength() const override { return 128; }
    bool isPasswordField() const override { return true; }
};

};  // namespace WingletUI

#endif // ROOTPASSWORDSETTING_H
