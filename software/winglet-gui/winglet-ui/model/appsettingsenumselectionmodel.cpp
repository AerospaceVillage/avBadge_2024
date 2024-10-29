#include "appsettingsenumselectionmodel.h"
#include <QPalette>

namespace WingletUI {

QModelIndex AppSettingsEnumSelectionModel::getIndexForPropVal(int val) const
{
    for (int i = 0; i < valueLookup.count(); i++) {
        if (valueLookup[i].first == val) {
            return createIndex(i, 0);
        }
    }

    // Couldn't find it, return invalid index
    return {};
}

int AppSettingsEnumSelectionModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())  // If parent is invalid, return root child count
        return valueLookup.size();
    else
        return 0; // Report no children to any items
}

QVariant AppSettingsEnumSelectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        if (role == Qt::DisplayRole) {
            // For root node, report title so scrollable list shows the title
            return title;
        }
        else {
            return {};
        }
    }

    if (index.row() >= valueLookup.size() || index.column() != 0 || index.parent().isValid()) {
        // Make sure the index points to a valid item
        return {};
    }

    if (role == Qt::DisplayRole) {
        return valueLookup.at(index.row()).second;
    }
    else if (role == Qt::UserRole) {
        return valueLookup.at(index.row()).first;
    }
    else if (role == Qt::ForegroundRole) {
        if (index.row() == selectedValueIdx)
            return QPalette::Link;
        else
            return {};
    }
    else {
        return {};
    }
}

void AppSettingsEnumSelectionModel::setSelectedValue(QModelIndex idx)
{
    // Don't refresh if there isn't a change
    if (idx.isValid() && idx.row() == selectedValueIdx)
        return;
    if (!idx.isValid() && selectedValueIdx == -1)
        return;

    // Update the index
    int oldIndex = selectedValueIdx;
    if (idx.isValid())
        selectedValueIdx = idx.row();
    else
        selectedValueIdx = -1;

    // Report the changed rows
    QModelIndex topLeft;
    QModelIndex bottomRight;
    if (oldIndex >= 0 && selectedValueIdx >= 0) {
        // Both are valid, set top left/bottom right accordingly
        if (oldIndex > selectedValueIdx) {
            topLeft = createIndex(selectedValueIdx, 0);
            bottomRight = createIndex(oldIndex, 0);
        }
        else {
            topLeft = createIndex(oldIndex, 0);
            bottomRight = createIndex(selectedValueIdx, 0);
        }
    }
    else if (oldIndex >= 0) {
        // Only old is valid, just that row changed
        topLeft = createIndex(oldIndex, 0);
        bottomRight = topLeft;
    }
    else {
        // Must be that only new index is valid, just update that one
        topLeft = createIndex(selectedValueIdx, 0);
        bottomRight = topLeft;
    }

    emit dataChanged(topLeft, bottomRight, {Qt::ForegroundRole});
}

} // namespace WingletUI
