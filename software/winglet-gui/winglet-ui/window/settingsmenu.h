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
    explicit SettingsMenu(QWidget *parent = nullptr, bool isCanardSettings = false);

protected:
    void focusInEvent(QFocusEvent* ev) override;
    void showEvent(QShowEvent *event) override;

protected slots:
    void menuItemSelected(QModelIndex index);
    void menuExitRequested();
    void menuExitConfirmClicked(int btnIdx);
    void selectorIndexSelected(QModelIndex index);
    void cicularKbdTextEntered(QString val);
    void colorPaletteChanged();
    void msgboxBtnClicked(int btnIdx);

    void canardConnectionChanged(bool connected);

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
        ACTION_STATE_WIFI_SCAN_RETURN,
        ACTION_STATE_MSGBOX_RETURN,
        ACTION_STATE_MANAGE_NETWORKS_RETURN,
        ACTION_STATE_INFO_RETURN,
        ACTION_STATE_RESET_ROOT_PASSWORD_CONFIRM,
        ACTION_STATE_COLD_BOOT_GPS_CONFIRM,
        ACTION_STATE_MENU_CLOSE,
        ACTION_STATE_SHOW_INVALID_CANARD_SETTINGS,
        ACTION_STATE_SHOW_CANARD_SETTINGS_SAVE_FAIL,
    };

    SettingsActionState actionState = ACTION_STATE_MENU_OPEN;

    QLabel *avLogoLabel;
    StatusBar *statusBar;
    ScrollableMenu *menuWidget;
    SettingsMenuModel *menuModel;

    AbstractSettingsEntry *activeItem;
    QString enteredWifiSSID;
    bool wifiScanAskPsk;
    int selectedMsgboxBtnIdx;
    bool isCanardSettings;
};

} // namespace WingletUI

#endif // WINGLETUI_SETTINGSAPP_H
