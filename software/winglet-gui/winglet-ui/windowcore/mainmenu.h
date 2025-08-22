#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include "winglet-ui/model/appmenumodel.h"
#include "winglet-ui/widget/scrollablemenu.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    MainMenu(QWidget *parent = nullptr);
    ~MainMenu();

protected:
    void focusInEvent(QFocusEvent* ev) override;

    void showEvent(QShowEvent *event) override;

protected slots:
    void menuItemSelected(QModelIndex index);
    void menuBeginningHide(unsigned int duration);
    void menuBeginningShow(unsigned int duration);
    void colorPaletteChanged();
    void canardConnectionChanged(bool connected);

private:
    ScrollableMenu *menuWidget;
    AppMenuModel *menuModel;
    StatusBar *statusBar;
    QPropertyAnimation *statusBarOpacityAnimation;
    QLabel *avLogoLabel;

    enum MenuActionState {
        ACTION_STATE_FIRST_START,
        ACTION_STATE_RELEASE_NOTES_DONE,
        ACTION_STATE_RETURN,
        ACTION_STATE_GAMEBOY_START
    };

    MenuActionState actionState = ACTION_STATE_FIRST_START;
};

} // namespace WingletUI

#endif // MAINMENU_H
