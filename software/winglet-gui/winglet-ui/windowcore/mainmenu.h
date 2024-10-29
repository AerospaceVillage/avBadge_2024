#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include "winglet-ui/model/appmenumodel.h"
#include "winglet-ui/widget/scrollablemenu.h"
#include "winglet-ui/widget/statusbar.h"

#define RELEASE_NOTES_FLAG_FILE "/var/show_release_notes"
#define RELEASE_NOTES_LOCATION "/etc/release_notes"

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

private:
    ScrollableMenu *menuWidget;
    AppMenuModel *menuModel;
    StatusBar *statusBar;
    QPropertyAnimation *statusBarOpacityAnimation;
    QLabel *avLogoLabel;

    bool tryShowReleaseNotes();

    enum MenuActionState {
        ACTION_STATE_FIRST_START,
        ACTION_STATE_RELEASE_NOTES_DONE,
        ACTION_STATE_RETURN
    };

    MenuActionState actionState = ACTION_STATE_FIRST_START;
};

} // namespace WingletUI

#endif // MAINMENU_H
