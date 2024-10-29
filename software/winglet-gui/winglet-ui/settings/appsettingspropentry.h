#ifndef APPSETTINGSPROPENTRY_H
#define APPSETTINGSPROPENTRY_H

#include "abstractsettingsentry.h"
#include "winglet-ui/model/appsettingsenumselectionmodel.h"
#include <QMetaProperty>
#include <QVector>

namespace WingletUI {

class AppSettingsBoolPropEntry : public AbstractBoolSetting
{
    Q_OBJECT
public:
    AppSettingsBoolPropEntry(QObject *propOwner, const char* propName, QString displayName);
    void setValue(bool val) override;
    bool value() const override;

protected slots:
    void settingUpdated(bool newVal);

private:
    QObject *propOwner;
    QMetaProperty prop;
};

class AppSettingsEnumPropEntry : public AbstractListSetting
{
    Q_OBJECT
public:
    AppSettingsEnumPropEntry(QObject *propOwner, const char* propName, QString displayName,
                             const QVector<QPair<int, QString>>& valueMap, QPixmap icon = {});

    QAbstractItemModel* selectionModel() override { return model; }
    QString curValueName() const override;
    QModelIndex curIndex() const override { return curIndexInternal(); }
    void setValue(QModelIndex idx) override;

protected slots:
    void settingUpdated(int newVal);

private:
    QModelIndex curIndexInternal() const;
    AppSettingsEnumSelectionModel *model;
    QObject *propOwner;
    QMetaProperty prop;
};

class AppSettingsDoublePropEntry : public AbstractTextSetting
{
    Q_OBJECT
public:
    AppSettingsDoublePropEntry(QObject *propOwner, const char * propName, QString displayName,
                               QString prompt, QPixmap icon = {}, double minValue = 0, double maxValue = 0);
    // If both minValue and maxValue are 0, it disables the input validator

    QString value() const override;
    void setValue(const QString &val) override;
    QList<QString> inputKeys() const override { return kbdChars; }
    QString prompt() const override;
    int maxLength() const override;
    QString title() const override;
    QValidator* validator() const override;
    bool allowEmptyInput() const override;
    QString validatorFailedMsg() const override;

protected slots:
    void settingUpdated(double newVal);

private:
    QList<QString> kbdChars = {"0123456789.-"};
    QObject *propOwner;
    QString m_prompt;
    double minValue;
    double maxValue;
    QValidator *m_validator;
    QMetaProperty prop;
};

} // namespace WingletUI

#endif // APPSETTINGSPROPENTRY_H
