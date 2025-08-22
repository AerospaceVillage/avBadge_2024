#include "appmenumodel.h"

namespace WingletUI {

static void searchMenuChildren(QHash<const AppMenuItem*, MenuMapLookup> *map, const AppMenuItem* menu, size_t parentIndex)
{
    // Assumes that submenu is non-null before call
    for (size_t i = 0; i < menu->numChildren; i++) {
        map->insert(&menu->submenu[i], {menu, parentIndex});
        if (menu->submenu[i].submenu) {
            searchMenuChildren(map, &menu->submenu[i], i);
        }
    }
}

AppMenuModel::AppMenuModel(const AppMenuItem* menuRoot, QObject *parent)
    : QAbstractItemModel{parent}, menuRoot(menuRoot)
{
    // Populate the parent hashmap
    parentMap = new QHash<const AppMenuItem*, MenuMapLookup>();

    assert(menuRoot->submenu);  // Menu root MUST have children
    searchMenuChildren(parentMap, menuRoot, 0);
}

AppMenuModel::~AppMenuModel()
{
    delete parentMap;
}

QVariant AppMenuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        // If passed invalid index, assume they meant the root node
        if (role == Qt::DisplayRole) {
            // Return the menu root title
            return menuRoot->title;
        }
        else {
            // All other roles are null
            return {};
        }
    }
    else {
        const AppMenuItem* node = (const AppMenuItem*) index.internalPointer();

        if (role == Qt::DisplayRole) {
            return node->title;
        }
        else if (role == Qt::UserRole && node->submenu == NULL) {
            // Only has user role if not a submenu
            return node->type;
        }
        else {
            return {};
        }
    }
}

QModelIndex AppMenuModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0) {
        return {};
    }

    const AppMenuItem* node = menuRoot;
    if (parent.isValid())
        node = (const AppMenuItem*) parent.internalPointer();

    if (!node->submenu) {
        // Not a submenu, can't get children
        return {};
    }

    int numVisibleChildren = node->numChildren;
    if (lastMainEntryDisabled && !parent.isValid()) {
        // Removes an item from the array if last item disabled
        numVisibleChildren--;
    }

    if (row < 0 || row >= numVisibleChildren) {
        // Row not inside submenu list
        return {};
    }

    // Valid, return index to that row
    return createIndex(row, 0, (void*)(&node->submenu[row]));
}

QModelIndex AppMenuModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto item = parentMap->find((const AppMenuItem*) index.internalPointer());
    if (item == parentMap->end()) {
        return {};
    }
    else if (item->parent == menuRoot) {
        return {};
    }
    else {
        return createIndex(item->parentIndex, 0, (void*) item->parent);
    }
}

int AppMenuModel::rowCount(const QModelIndex &parent) const
{
    // Get the node for the index
    const AppMenuItem* node = menuRoot;
    if (parent.isValid())
        node = (const AppMenuItem*) parent.internalPointer();

    if (node->submenu) {
        // If submenu defined, its a menu and return number of children
        int numVisibleChildren = node->numChildren;
        if (lastMainEntryDisabled && !parent.isValid()) {
            // Removes an item from the array if last item disabled
            numVisibleChildren--;
        }
        return numVisibleChildren;
    }
    else {
        // If not it's a menu entry, return no children
        return 0;
    }
}

int AppMenuModel::columnCount(const QModelIndex &parent) const
{
    (void) parent;
    return 1;  // Always 1 column
}

Qt::ItemFlags AppMenuModel::flags(const QModelIndex &index) const
{
    (void) index;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void AppMenuModel::disableLastMainEntry(bool disableEntry) {
    if (lastMainEntryDisabled == disableEntry) {
        return;
    }

    // 1. Prepare for scan results refresh
    emit layoutAboutToBeChanged();

    // 2. Save previous state so we can update the persistent indices
    QModelIndex prevLastEntry = index(menuRoot->numChildren - 1, 0);

    // 3. Modify Model State
    lastMainEntryDisabled = disableEntry;

    // 4. Update the persistent model indices
    if (disableEntry) {
        // Only needs update if we're removing the last entry
        changePersistentIndex(prevLastEntry, {});
    }

    // 5. Notify of layout changed
    emit layoutChanged();
}

} // namespace WingletUI
