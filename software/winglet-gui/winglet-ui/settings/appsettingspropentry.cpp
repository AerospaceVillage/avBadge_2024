#include "appsettingspropentry.h"
#include <QDoubleValidator>

namespace WingletUI {

// ========================================
// AppSettings Bool Property Entry
// ========================================

AppSettingsBoolPropEntry::AppSettingsBoolPropEntry(QObject *propOwner, const char* propName, QString displayName):
    AbstractBoolSetting(propOwner, displayName), propOwner(propOwner) {

    const QMetaObject* meta = propOwner->metaObject();
    int propIdx = meta->indexOfProperty(propName);
    if (propIdx < 0) {
        qWarning("AppSettingsBoolPropEntry: Failed to locate property '%s' in class '%s'", propName, meta->className());
        return;
    }

    QMetaProperty propInfo = meta->property(propIdx);
    if (!propInfo.isReadable() || !propInfo.isWritable()) {
        qWarning("AppSettingsBoolPropEntry: Cannot initialize using property '%s' from class '%s': Not readable/writable", propName, meta->className());
        return;
    }
    if (propInfo.type() != QVariant::Bool) {
        qWarning("AppSettingsBoolPropEntry: Cannot initialize using property '%s' from class '%s': Not a bool", propName, meta->className());
        return;
    }
    prop = propInfo;

    // Connect the settings updated signal the property
    QMetaObject::connect(propOwner, propInfo.notifySignalIndex(), this, metaObject()->indexOfSlot("settingUpdated(bool)"));
}

void AppSettingsBoolPropEntry::settingUpdated(bool newVal) {
    (void) newVal;
    reportChanged();
}

void AppSettingsBoolPropEntry::setValue(bool val) {
    if (!prop.isValid())
        return;

    prop.write(propOwner, val);
}

bool AppSettingsBoolPropEntry::value() const {
    if (!prop.isValid())
        return false;

    QVariant val = prop.read(propOwner);
    return val.toBool();
}

// ========================================
// AppSettings Enum Property Entry
// ========================================

AppSettingsEnumPropEntry::AppSettingsEnumPropEntry(QObject *propOwner, const char* propName, QString displayName,
                                                   const QVector<QPair<int, QString>>& valueMap, QPixmap icon):
        AbstractListSetting(propOwner, displayName, icon), model(new AppSettingsEnumSelectionModel(this, displayName, valueMap)),
        propOwner(propOwner) {

    const QMetaObject* meta = propOwner->metaObject();
    int propIdx = meta->indexOfProperty(propName);
    if (propIdx < 0) {
        qWarning("AppSettingsEnumPropEntry: Failed to locate property '%s' in class '%s'", propName, meta->className());
        return;
    }

    QMetaProperty propInfo = meta->property(propIdx);
    if (!propInfo.isReadable() || !propInfo.isWritable()) {
        qWarning("AppSettingsEnumPropEntry: Cannot initialize using property '%s' from class '%s': Not readable/writable", propName, meta->className());
        return;
    }
    if (propInfo.type() != QVariant::Int) {
        qWarning("AppSettingsEnumPropEntry: Cannot initialize using property '%s' from class '%s': Not an int", propName, meta->className());
        return;
    }
    prop = propInfo;

    // Connect the settings updated signal the property
    QMetaObject::connect(propOwner, propInfo.notifySignalIndex(), this, metaObject()->indexOfSlot("settingUpdated(int)"));

    // Refresh the current selected value of the model to reflect the active value
    model->setSelectedValue(curIndexInternal());
}

QString AppSettingsEnumPropEntry::curValueName() const
{
    auto modelIdx = curIndex();
    if (modelIdx.isValid()) {
        return model->data(modelIdx).toString();
    }
    else {
        return QString("Unknown (%1)").arg(prop.read(propOwner).toInt());
    }
}

QModelIndex AppSettingsEnumPropEntry::curIndexInternal() const
{
    bool convOk = false;
    QVariant valVariant = prop.read(propOwner);
    int val = valVariant.toInt(&convOk);
    if (!convOk) {
        qWarning("AppSettingsEnumPropEntry::curIndex() Failed to convert property '%s' to int", prop.name());
        return {};
    }

    return model->getIndexForPropVal(val);
}

void AppSettingsEnumPropEntry::setValue(QModelIndex idx)
{
    if (!idx.isValid())
        return;

    auto variantVal = model->data(idx, Qt::UserRole);
    if (!variantVal.isValid())
        return;

    bool convOk = false;
    int val = variantVal.toInt(&convOk);
    if (!convOk) {
        qWarning("AppSettingsEnumPropEntry::setValue: Model returned UserRole which was not an integer");
        return;
    }
    else {
        prop.write(propOwner, val);
    }
}

void AppSettingsEnumPropEntry::settingUpdated(int newVal)
{
    // Need to notify the selected settings model that it has a new value
    model->setSelectedValue(model->getIndexForPropVal(newVal));
    reportChanged();
}

// ========================================
// AppSettings Double Property Entry
// ========================================

AppSettingsDoublePropEntry::AppSettingsDoublePropEntry(QObject *propOwner, const char * propName, QString displayName,
                                                       QString prompt, QPixmap icon, double minValue, double maxValue):
    AbstractTextSetting(propOwner, displayName, icon), propOwner(propOwner), m_prompt(prompt),
    minValue(minValue), maxValue(maxValue)
{
    const QMetaObject* meta = propOwner->metaObject();
    int propIdx = meta->indexOfProperty(propName);
    if (propIdx < 0) {
        qWarning("AppSettingsEnumPropEntry: Failed to locate property '%s' in class '%s'", propName, meta->className());
        return;
    }

    QMetaProperty propInfo = meta->property(propIdx);
    if (!propInfo.isReadable() || !propInfo.isWritable()) {
        qWarning("AppSettingsEnumPropEntry: Cannot initialize using property '%s' from class '%s': Not readable/writable", propName, meta->className());
        return;
    }
    if (propInfo.type() != QVariant::Double) {
        qWarning("AppSettingsEnumPropEntry: Cannot initialize using property '%s' from class '%s': Not a double", propName, meta->className());
        return;
    }
    prop = propInfo;

    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setNotation(QDoubleValidator::StandardNotation);

    if (minValue != 0 || maxValue != 0) {
        validator->setRange(minValue, maxValue, -1);
    }

    m_validator = validator;

    // Connect the settings updated signal the property
    QMetaObject::connect(propOwner, propInfo.notifySignalIndex(), this, metaObject()->indexOfSlot("settingUpdated(double)"));
}
// If both minValue and maxValue are 0, it disables the input validator

QString AppSettingsDoublePropEntry::value() const
{
    if (!prop.isValid())
        return "0";

    QVariant val = prop.read(propOwner);
    return QString::number(val.toDouble());
}

void AppSettingsDoublePropEntry::setValue(const QString &val)
{
    if (!prop.isValid())
        return;

    bool convOkay;
    double valDouble = val.toDouble(&convOkay);
    if (!convOkay)
        return;

    prop.write(propOwner, valDouble);
}

void AppSettingsDoublePropEntry::settingUpdated(double newVal) {
    (void) newVal;
    reportChanged();
}

QString AppSettingsDoublePropEntry::prompt() const
{
    return m_prompt;
}

int AppSettingsDoublePropEntry::maxLength() const
{
    return 10;
}

QString AppSettingsDoublePropEntry::title() const
{
    return name();
}

QValidator* AppSettingsDoublePropEntry::validator() const
{
    return m_validator;
}

bool AppSettingsDoublePropEntry::allowEmptyInput() const
{
    return false;
}

QString AppSettingsDoublePropEntry::validatorFailedMsg() const
{
    if (minValue != 0 || maxValue != 0)
        return QString("Must be Decimal Number between %1 and %2").arg(minValue).arg(maxValue);
    else
        return "Must be Decimal Number";
}

} // namespace WingletUI
