#ifndef WINGLETUI_SELECTORBOX_H
#define WINGLETUI_SELECTORBOX_H

#include <QWidget>
#include <QAbstractItemModel>
#include "winglet-ui/widget/scrollablemenu.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

class SelectorBox : public QWidget
{
    Q_OBJECT
public:
    explicit SelectorBox(QWidget *parent, QAbstractItemModel *model, bool showStatusBar = false, int defaultIndex = 0);

signals:
    void indexSelected(QModelIndex index);

protected:
    void focusInEvent(QFocusEvent* ev) override;
    void showEvent(QShowEvent *event) override;

protected slots:
    void menuItemSelected(QModelIndex index);
    void menuExitRequested();

private:
    QLabel *avLogoLabel;
    StatusBar *statusBar;

public:
    ScrollableMenu *menuWidget;
};

} // namespace WingletUI

#endif // WINGLETUI_SELECTORBOX_H
