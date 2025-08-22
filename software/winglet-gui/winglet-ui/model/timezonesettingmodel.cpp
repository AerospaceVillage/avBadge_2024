#include "timezonesettingmodel.h"

#include <QDir>
#include <QFile>
#include <QTimeZone>
#include <QPalette>

#define ZONE_INFO_DIR "/usr/share/zoneinfo/"

namespace WingletUI {

class TimeZoneOptionItem {
public:
    TimeZoneOptionItem(TimeZoneOptionDir* parent, const QByteArray &name): parent(parent), name(name) {}
    virtual ~TimeZoneOptionItem() {}
    TimeZoneOptionDir* const parent;
    const QByteArray name;
    bool isActive = false;
};

class TimeZoneOptionValue: public TimeZoneOptionItem {
public:
    TimeZoneOptionValue(TimeZoneOptionDir* parent, const QByteArray &name, const QByteArray &tzId):
        TimeZoneOptionItem(parent, name), tzId(tzId) {}
    const QByteArray tzId;
};

class TimeZoneOptionDir: public TimeZoneOptionItem {
public:
    using TimeZoneOptionItem::TimeZoneOptionItem;
    ~TimeZoneOptionDir() {
        for (auto child : children) {
            delete child;
        }
    }

    TimeZoneOptionItem* createChildLeaf(const QByteArray &name, const QByteArray &tzId) {
        auto item = new TimeZoneOptionValue(this, name, tzId);
        int idx = children.length();
        children.append(item);
        childrenIdxMap.insert(name, idx);
        return item;
    }

    TimeZoneOptionDir* createChildDir(QByteArray name) {
        auto item = new TimeZoneOptionDir(this, name);
        int idx = children.length();
        children.append(item);
        childrenIdxMap.insert(name, idx);
        return item;
    }

    int count() const { return children.count(); }
    TimeZoneOptionItem* getChild(int idx) const {
        if (idx < 0 || idx >= children.count())
            return NULL;
        else
            return children.at(idx);
    }

    int getChildIdxByName(const QByteArray& name)
    {
        auto itr = childrenIdxMap.find(name);
        if (itr == childrenIdxMap.end()) {
            return -1;
        }
        else {
            return itr.value();
        }
    }

private:
    QList<TimeZoneOptionItem*> children;
    QMap<QByteArray, int> childrenIdxMap;
};



TimeZoneSettingModel::TimeZoneSettingModel(QObject *parent)
    : QAbstractItemModel{parent},
      rootOption(new TimeZoneOptionDir(NULL, "Timezone"))
{
    QByteArray systemTzId = QTimeZone::systemTimeZoneId();

    for (auto& tzId : QTimeZone::availableTimeZoneIds()) {
        if (!QFile::exists(TimeZoneSettingModel::tzIdToPath(tzId)))
            continue;

        bool isActive = (tzId == systemTzId);
        auto tzPath = tzId.split('/');

        TimeZoneOptionDir* dir = rootOption;
        for (int i = 0; i < tzPath.length(); i++) {
            if (i < tzPath.length() - 1) {
                // Find the path matching the name, if it doesn't exist, make it
                TimeZoneOptionItem* ptr = dir->getChild(dir->getChildIdxByName(tzPath.at(i) + "/"));
                if (!ptr) {
                     dir = dir->createChildDir(tzPath.at(i) + "/");
                }
                else {
                    dir = dynamic_cast<TimeZoneOptionDir*>(ptr);
                }
                if (isActive)
                    dir->isActive = true;
            }
            else {
                // Reached the end of the path, make a new leaf node for this item
                auto leaf = dir->createChildLeaf(tzPath.at(i), tzId);
                if (isActive)
                    leaf->isActive = isActive;
            }
        }
    }
}

TimeZoneSettingModel::~TimeZoneSettingModel() {
    delete rootOption;
}

QString TimeZoneSettingModel::tzIdToPath(const QString &tzId)
{
    return QDir::cleanPath(ZONE_INFO_DIR + tzId);
}

QModelIndex TimeZoneSettingModel::getIndexForPropVal(const QByteArray &tzId) const
{
    auto tzPath = tzId.split('/');

    TimeZoneOptionDir* dir = rootOption;
    for (int i = 0; i < tzPath.length(); i++) {
        if (i < tzPath.length() - 1) {
            // Follow the parent nodes
            TimeZoneOptionItem* ptr = dir->getChild(dir->getChildIdxByName(tzPath.at(i) + "/"));
            if (!ptr) {
                // Couldn't find item with that path, return invalid index
                return {};
            }
            else {
                dir = dynamic_cast<TimeZoneOptionDir*>(ptr);
            }
        }
        else {
            // Reached the end of the path, return the index of this item
            int idx = dir->getChildIdxByName(tzPath.at(i));
            TimeZoneOptionItem* ptr = dir->getChild(idx);

            // Couldn't find the item, return invalid index
            if (!ptr)
                return {};
            else
                return createIndex(idx, 0, ptr);
        }
    }
    return {};
}

QVariant TimeZoneSettingModel::data(const QModelIndex &index, int role) const
{
    // Invalid index is the root node
    if (!index.isValid()) {
        if (role == Qt::DisplayRole) {
            // Set the title for the menu
            return "Timezone";
        }
        else {
            return {};
        }
    }

    // If not, get the actual data
    TimeZoneOptionItem* entry = (TimeZoneOptionItem*) index.internalPointer();

    if (role == Qt::DisplayRole) {
        return QString(entry->name);
    }
    else if (role == Qt::ForegroundRole) {
        if (entry->isActive)
            return QPalette::Link;
        else
            return {};
    }
    else if (role == Qt::UserRole) {
        // User role has the path to the timezone entry if it is valid
        TimeZoneOptionValue* value = dynamic_cast<TimeZoneOptionValue*>(entry);
        if (value)
            return value->tzId;
        else
            return {};
    }
    else {
        return {};
    }
}

QModelIndex TimeZoneSettingModel::index(int row, int column, const QModelIndex &parent) const
{
    // Only column 0 allowed
    if (column != 0)
        return {};

    TimeZoneOptionDir* parentDir;
    if (parent.isValid()) {
        TimeZoneOptionItem *ptr = (TimeZoneOptionItem*) parent.internalPointer();
        parentDir = dynamic_cast<TimeZoneOptionDir*>(ptr);
        // Can't create child index of a leaf node
        if (!parentDir)
            return {};
    }
    else {
        parentDir = rootOption;
    }

    if (row < 0 || row >= parentDir->count())
        return {};

    return createIndex(row, column, parentDir->getChild(row));
}

QModelIndex TimeZoneSettingModel::parent(const QModelIndex &index) const {
    if (!index.isValid())
        return {};

    TimeZoneOptionItem* entry = (TimeZoneOptionItem*) index.internalPointer();
    TimeZoneOptionDir* parent = entry->parent;
    // No parent, then it's the rootOption (shouldn't be possible, but better to be safe than dereference a null ptr)
    if (!parent)
        return {};

    // Need to find the index of the parent in the parent
    TimeZoneOptionDir* grandparent = parent->parent;
    if (!grandparent)
        return {};  // If no grandparent then the parent is the rootOption (so it'll have a null model index)

    // Quickest way is to look up index by name so we don't have to do a search
    int idx = grandparent->getChildIdxByName(parent->name);
    if (idx < 0)
        return {};

    // Create the parent index
    return createIndex(idx, 0, parent);
}

int TimeZoneSettingModel::rowCount(const QModelIndex &parent) const
{
    TimeZoneOptionDir* parentDir;
    if (parent.isValid()) {
        TimeZoneOptionItem *ptr = (TimeZoneOptionItem*) parent.internalPointer();
        parentDir = dynamic_cast<TimeZoneOptionDir*>(ptr);
        // parent is a leaf node, no children available
        if (!parentDir)
            return 0;
    }
    else {
        parentDir = rootOption;
    }

    return parentDir->count();
}

int TimeZoneSettingModel::columnCount(const QModelIndex &parent) const
{
    (void) parent;
    return 1;
}

} // namespace WingletUI
