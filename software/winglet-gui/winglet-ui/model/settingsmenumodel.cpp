#include "settingsmenumodel.h"

Q_DECLARE_METATYPE(WingletUI::AbstractSettingsEntry*)

namespace WingletUI {

SettingsMenuModel::SettingsMenuModel(const SettingsEntryContainer *menuRoot, QObject *parent)
    : QAbstractItemModel{parent}, menuRoot(menuRoot)
{
    connect(menuRoot, SIGNAL(childChanged(QObject*)), this, SLOT(settingsEntryChanged(QObject*)));
}

void SettingsMenuModel::settingsEntryChanged(QObject *obj) {
    auto entry = dynamic_cast<AbstractSettingsEntry*>(obj);
    if (entry) {
        QModelIndex index = getObjectIndex(entry);

        // Emit data changed for this object
        // Not entirely sure what changed, just emit all the roles that can be determined by a setting value
        emit dataChanged(index, index, {Qt::EditRole, Qt::CheckStateRole});
    }
}

QModelIndex SettingsMenuModel::getObjectIndex(AbstractSettingsEntry *entry) const {
    // Need to get parent to get entry parent
    auto parent = dynamic_cast<SettingsEntryContainer*>(entry->parent());
    if (!parent) {
        // No parent, the entry must be the root model index (aka empty QModelIndex)
        return {};
    }

    // Get index of entry in its parent
    int entryIndex = parent->entries().indexOf(entry);
    return createIndex(entryIndex, 0, (void*)entry);
}

QVariant SettingsMenuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        // If passed invalid index, assume they meant the root node
        if (role == Qt::DisplayRole) {
            // Return the menu root title
            return menuRoot->name();
        }
        else {
            // All other roles are null
            return {};
        }
    }
    else {
        AbstractSettingsEntry* node = (AbstractSettingsEntry*) index.internalPointer();
        if (!node)
            return {};

        if (role == Qt::DisplayRole) {
            return node->name();
        }
        else if (role == Qt::CheckStateRole) {
            // Check state can be returned for boolean items
            if (auto boolSetting = dynamic_cast<AbstractBoolSetting*>(node)) {
                return boolSetting->value() ? Qt::Checked : Qt::Unchecked;
            }
            else {
                return {};
            }
        }
        else if (role == Qt::DecorationRole) {
            return node->icon();
        }
        else if (role == Qt::EditRole) {
            // Enum values can have their current value returned
            if (auto listSetting = dynamic_cast<AbstractListSetting*>(node)) {
                return listSetting->curValueName();
            }
            else if (auto textSetting = dynamic_cast<AbstractTextSetting*>(node)) {
                return textSetting->value();
            }
            else {
                return {};
            }
        }
        else if (role == Qt::UserRole) {
            // User role is just a pointer to the AbstractSettingsEntry that can be controlled by caller
            return QVariant::fromValue(node);
        }
        else {
            return {};
        }
    }
}

bool SettingsMenuModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        // Can't set data on invalid index
        return false;
    }

    AbstractSettingsEntry* node = (AbstractSettingsEntry*) index.internalPointer();
    if (!node)
        return false;

    if (role == Qt::CheckStateRole) {
        // Value must be a valid check state
        if (!value.canConvert<Qt::CheckState>()) {
            return false;
        }

        if (auto boolSetting = dynamic_cast<AbstractBoolSetting*>(node)) {
            Qt::CheckState newCheckState = value.value<Qt::CheckState>();
            bool newValue;
            if (newCheckState == Qt::Checked) {
                newValue = true;
            }
            else if (newCheckState == Qt::Unchecked) {
                newValue = false;
            }
            else {
                // Only accept checked or unchecked, else fail
                return false;
            }

            boolSetting->setValue(newValue);
            return true;
        }
        else {
            // Couldn't cast setting, must not be a checkbox
            return false;
        }
    }

    return false;
}

Qt::ItemFlags SettingsMenuModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled;

    if (index.isValid()) {
        AbstractSettingsEntry* node = (AbstractSettingsEntry*) index.internalPointer();
        if (dynamic_cast<AbstractBoolSetting*>(node)) {
            flags |= Qt::ItemIsUserCheckable;
        }
    }

    return flags;
}

QModelIndex SettingsMenuModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0) {
        return {};
    }

    const SettingsEntryContainer* container = menuRoot;
    if (parent.isValid()) {
        AbstractSettingsEntry *parentEntry = (AbstractSettingsEntry*) parent.internalPointer();
        container = dynamic_cast<SettingsEntryContainer*>(parentEntry);
        if (!container) {
            // parent is not a submenu (couldn't cast to settings entry container), can't get children
            return {};
        }
    }

    auto& childrenList = container->entries();
    if (row < 0 || row >= childrenList.length()) {
        // Row not valid index into list
        return {};
    }

    // Valid, return index to that row
    return createIndex(row, 0, (void*)(childrenList.at(row)));
}

QModelIndex SettingsMenuModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }

    AbstractSettingsEntry *entry = (AbstractSettingsEntry*) index.internalPointer();
    auto parent = dynamic_cast<SettingsEntryContainer*>(entry->parent());
    if (!parent) {
        // No parent, we must be the root model index
        return {};
    }

    // Create index for the parent object
    return getObjectIndex(parent);
}

int SettingsMenuModel::rowCount(const QModelIndex &parent) const
{
    // Get the node for the index
    const SettingsEntryContainer* container = menuRoot;
    if (parent.isValid()) {
        AbstractSettingsEntry *parentEntry = (AbstractSettingsEntry*) parent.internalPointer();
        container = dynamic_cast<SettingsEntryContainer*>(parentEntry);
        if (!container) {
            // parent is not a submenu (couldn't cast to settings entry container), so 0 children present
            return 0;
        }
    }

    return container->entries().length();
}

int SettingsMenuModel::columnCount(const QModelIndex &parent) const
{
    (void) parent;
    return 1;  // Always 1 column
}

} // namespace WingletUI
