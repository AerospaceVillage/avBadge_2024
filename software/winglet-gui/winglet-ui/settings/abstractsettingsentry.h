#ifndef ABSTRACTSETTINGSENTRY_H
#define ABSTRACTSETTINGSENTRY_H

#include <QObject>
#include <QPixmap>
#include <QList>

class QValidator;
class QAbstractItemModel;

namespace WingletUI {

class AbstractSettingsEntry : public QObject
{
    Q_OBJECT
public:
    explicit AbstractSettingsEntry(QObject *parent, QString name, QPixmap icon = {}):
        QObject{parent}, m_name(name), m_icon(icon) {}

    QString name() const { return m_name; }
    QPixmap icon() const { return m_icon; }

protected:
    virtual void reportEntryChanged(AbstractSettingsEntry* entry) {
        auto parentEntry = dynamic_cast<AbstractSettingsEntry*>(parent());
        if (parentEntry != nullptr)
            parentEntry->reportEntryChanged(entry);
    }
    void reportChanged() { reportEntryChanged(this); }

private:
    QString m_name;
    QPixmap m_icon;
};

class SettingsEntryContainer : public AbstractSettingsEntry {
    Q_OBJECT
public:
    using AbstractSettingsEntry::AbstractSettingsEntry;

    const QList<AbstractSettingsEntry*>& entries() const { return m_entries; }
    void addEntry(AbstractSettingsEntry* entry) {
        entry->setParent(this);
        m_entries.append(entry);
    }

signals:
    // Only emitted if this does not have a parent container
    void childChanged(QObject* entry);

protected:
    void reportEntryChanged(AbstractSettingsEntry* entry) override {
        emit childChanged(entry);
        AbstractSettingsEntry::reportEntryChanged(entry);
    }

private:
    QList<AbstractSettingsEntry*> m_entries;
};

class SettingsActionEntry : public AbstractSettingsEntry {
public:
    explicit SettingsActionEntry(QObject *parent, QString name, int action, QPixmap icon = {}):
        AbstractSettingsEntry(parent, name, icon), m_action(action) {}

    int action() const { return m_action; }

private:
    int m_action;
};

class AbstractBoolSetting : public AbstractSettingsEntry
{
public:
    explicit AbstractBoolSetting(QObject *parent, QString name): AbstractSettingsEntry(parent, name) {}

    virtual int value() const = 0;
    virtual void setValue(bool val) = 0;
    virtual void toggle() { int val = value(); if (val >= 0) setValue(!val); }
};

class AbstractListSetting : public AbstractSettingsEntry
{
public:
    using AbstractSettingsEntry::AbstractSettingsEntry;

    virtual QAbstractItemModel* selectionModel() = 0;
    virtual QString curValueName() const = 0;
    virtual QModelIndex curIndex() const = 0;
    virtual void setValue(QModelIndex idx) = 0;
};

class AbstractTextSetting : public AbstractSettingsEntry
{
public:
    using AbstractSettingsEntry::AbstractSettingsEntry;

    virtual QString value() const = 0;
    virtual QString displayValue() const { return value(); }
    virtual void setValue(const QString &val) = 0;

    // These control properties for the underlying circular keyboard
    virtual QList<QString> inputKeys() const = 0;
    virtual QString prompt() const = 0;
    virtual int maxLength() const { return -1; }
    virtual QString title() const { return ""; }
    virtual QValidator* validator() const { return nullptr; }
    virtual bool allowEmptyInput() const { return false; }
    virtual QString validatorFailedMsg() const { return ""; }
    virtual bool isPasswordField() const { return false; }
};

} // namespace WingletUI

#endif // ABSTRACTSETTINGSENTRY_H
