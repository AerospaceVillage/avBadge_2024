#ifndef WINGLETUI_APPMENUMODEL_H
#define WINGLETUI_APPMENUMODEL_H

#include <QAbstractItemModel>

namespace WingletUI {

struct AppMenuItem {
    const char* title;
    const AppMenuItem *submenu;
    union {
        size_t numChildren;  // Set if submenu is non-null
        int type;            // Set if submenu is null, and is returned in the menu callback (great for an enum)
    };
};

struct MenuMapLookup {
    const AppMenuItem* parent;
    size_t parentIndex;
};

class AppMenuModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AppMenuModel(const AppMenuItem* menuRoot, QObject *parent = nullptr);
    ~AppMenuModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // This is more of a hack to allow the Canard item to be added/removed
    // But this is the only thing that needs to be changed and isn't worth rewriting this whole class
    void disableLastMainEntry(bool disableEntry);

private:
    const AppMenuItem* menuRoot;
    QHash<const AppMenuItem*, MenuMapLookup> *parentMap;  // Map for children items to parents
    bool lastMainEntryDisabled = false;  // Hack to allow canard to be added/removed easily
};

} // namespace WingletUI

#endif // WINGLETUI_APPMENUMODEL_H
