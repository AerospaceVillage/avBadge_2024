#ifndef WINGLETUI_SETTINGSAPP_H
#define WINGLETUI_SETTINGSAPP_H

#include <QWidget>

#include "winglet-ui/model/settingsmenumodel.h"
#include "winglet-ui/widget/scrollablemenu.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

class SettingsMenu : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsMenu(QWidget *parent = nullptr);

protected:
    void focusInEvent(QFocusEvent* ev) override;
    void showEvent(QShowEvent *event) override;

protected slots:
    void menuItemSelected(QModelIndex index);
    void menuExitRequested();
    void selectorIndexSelected(QModelIndex index);
    void cicularKbdTextEntered(QString val);

private:
    enum SettingsActionState {
        ACTION_STATE_MENU_OPEN,
        ACTION_STATE_SELECTOR_RETURN,
        ACTION_STATE_REBOOT,
        ACTION_STATE_GET_DISCORD_USERNAME,
        ACTION_STATE_SHOW_DISCORD_MSGBOX,
        ACTION_STATE_TEXT_RETURN,
        ACTION_STATE_WIFI_SSID_RETURN,
        ACTION_STATE_WIFI_PSK_RETURN,
        ACTION_STATE_MSGBOX_RETURN,
        ACTION_STATE_MANAGE_NETWORKS_RETURN,
        ACTION_STATE_INFO_RETURN
    };

    SettingsActionState actionState = ACTION_STATE_MENU_OPEN;

    QLabel *avLogoLabel;
    StatusBar *statusBar;
    ScrollableMenu *menuWidget;
    SettingsMenuModel *menuModel;

    AbstractSettingsEntry *activeItem;
    QString enteredWifiSSID;
};

} // namespace WingletUI

#endif // WINGLETUI_SETTINGSAPP_H
