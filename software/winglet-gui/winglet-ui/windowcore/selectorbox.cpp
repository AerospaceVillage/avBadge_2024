#include "selectorbox.h"
#include "winglet-ui/theme.h"
#include "wingletgui.h"

#include <QFocusEvent>

namespace WingletUI {

SelectorBox::SelectorBox(QWidget *parent, QAbstractItemModel *model, bool showStatusBar, int defaultIndex)
    : QWidget{parent}, avLogoLabel(new QLabel(this)), menuWidget(new ScrollableMenu(this))
{
    menuWidget->setMenuWrap(false);
    menuWidget->setMaxVisibleItems(9);
    menuWidget->setShowShrinkFromOutside(false);
    menuWidget->setShrinkOnSelect(true);
    menuWidget->setFontSizes(22, 14, 1);
    menuWidget->setGeometry(0, 0, 480, 480);
    menuWidget->setModel(model);
    menuWidget->setCurrentIndex(defaultIndex);
    connect(menuWidget, SIGNAL(itemSelected(QModelIndex)), this, SLOT(menuItemSelected(QModelIndex)));
    connect(menuWidget, SIGNAL(menuExit()), this, SLOT(menuExitRequested()));

    // Create AV Logo in background
    activeTheme->renderBgAvLogo(avLogoLabel);

    // Show status bar if enabled
    if (showStatusBar) {
        statusBar = new StatusBar(this);
        statusBar->raise();
    }
}

void SelectorBox::showEvent(QShowEvent *event)
{
    (void) event;

    // Bubble show event to the menu widget so we get the cool animations
    menuWidget->show();
}

void SelectorBox::focusInEvent(QFocusEvent* ev)
{
    // Pass focus events to the underlying menu widget so it receives key events
    menuWidget->setFocus(ev->reason());
}

void SelectorBox::menuItemSelected(QModelIndex index)
{
    emit indexSelected(index);
    WingletGUI::inst->removeWidgetOnTop(this);
}

void SelectorBox::menuExitRequested()
{
    WingletGUI::inst->removeWidgetOnTop(this);
}

} // namespace WingletUI
