#include "settingsmenu.h"

#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include "winglet-ui/hardware/canardinterface.h"
#include "winglet-ui/model/knownnetworksmodel.h"
#include "winglet-ui/model/wifiscanmodel.h"
#include "winglet-ui/settings/appsettingspropentry.h"
#include "winglet-ui/settings/canardsettings.h"
#include "winglet-ui/settings/rootpasswordsetting.h"
#include "winglet-ui/windowcore/circularkeyboard.h"
#include "winglet-ui/windowcore/infoviewer.h"
#include "winglet-ui/windowcore/selectorbox.h"
#include "winglet-ui/windowcore/messagebox.h"
#include <QFocusEvent>

#define DISCORD_USERNAME_FILE "/tmp/attestation_username"

const static QRegExp discord_regexp("^(?=.{2,32}$)(?!(?:everyone|here)$)\\.?[a-z0-9_]+(?:\\.[a-z0-9_]+)*\\.?$");

namespace WingletUI {

class WifiPSKValidator: public QValidator {
    using QValidator::QValidator;

    State validate(QString & msg, int & posOut) const override {
        posOut = 0;

        size_t msgLen = msg.length();
        if (msgLen == 0 || (msgLen >= 8 && msgLen <= 63))
            return Acceptable;
        else if (msgLen < 8)
            return Intermediate;
        else
            return Invalid;
    }
};

SettingsMenu::SettingsMenu(QWidget *parent, bool isCanardSettings)
    : QWidget{parent}, avLogoLabel(new QLabel(this)),
      statusBar(new StatusBar(this)), menuWidget(new ScrollableMenu(this)),
      isCanardSettings(isCanardSettings)
{
    menuWidget->setMenuWrap(false);
    menuWidget->setMaxVisibleItems(9);
    menuWidget->setFontSizes(22, 14, 1);
    menuWidget->setGeometry(0, 0, 480, 480);

    if (isCanardSettings) {
        CanardSettings* settings = WingletGUI::inst->saoMonitor->canard->settings;
        if (settings->settingsValid()) {
            menuModel = new SettingsMenuModel(settings->settingsEntryRoot(), this);
            menuWidget->setModel(menuModel);
        }
        else {
            actionState = ACTION_STATE_SHOW_INVALID_CANARD_SETTINGS;
        }
    }
    else {
        menuModel = new SettingsMenuModel(WingletGUI::inst->settings.settingsEntryRoot(), this);
        menuWidget->setModel(menuModel);
    }
    connect(menuWidget, SIGNAL(itemSelected(QModelIndex)), this, SLOT(menuItemSelected(QModelIndex)));
    connect(menuWidget, SIGNAL(menuExit()), this, SLOT(menuExitRequested()));

    // Create AV Logo in background
    activeTheme->renderBgAvLogo(avLogoLabel);
    // Subscribe to color palette change events so the bg logo is updated if dark mode is changed
    connect(activeTheme, SIGNAL(colorPaletteChanged()), this, SLOT(colorPaletteChanged()));

    // Status bar should appear on top
    statusBar->raise();

    if (isCanardSettings) {
        connect(WingletGUI::inst->saoMonitor->canard, SIGNAL(connectionStateChanged(bool)), this, SLOT(canardConnectionChanged(bool)));
    }
}

void SettingsMenu::colorPaletteChanged()
{
    // Rerender the AV badge logo to update palette
    activeTheme->renderBgAvLogo(avLogoLabel);
}

void SettingsMenu::showEvent(QShowEvent *event)
{
    (void) event;

    // Immediately remove this widget if for canard and we're coming back to settings after unplug
    if (isCanardSettings && !WingletGUI::inst->saoMonitor->canard->connected()) {
        WingletGUI::inst->removeWidgetOnTop(this);
        return;
    }

    menuWidget->setShowShrinkFromOutside(true);

    if (WingletGUI::inst->settings.rebootNeeded && actionState != ACTION_STATE_REBOOT) {
        WingletGUI::inst->showMessageBox("A reboot is required after setting this item.\nPress the knob/A to reboot.", "Reboot Required", "Reboot");
        actionState = ACTION_STATE_REBOOT;
        return;
    }

    // Change menu fly in direction depending on what we're returning from
    switch (actionState) {
    case ACTION_STATE_MENU_OPEN:
        menuWidget->setShowShrinkFromOutside(false);
        break;

    case ACTION_STATE_REBOOT:
        QApplication::exit(EXIT_CODE_REBOOT);
        return;

    case ACTION_STATE_SELECTOR_RETURN:
    case ACTION_STATE_TEXT_RETURN:
        // Need to clear the active settings item being edited
        activeItem = nullptr;
        break;

    case ACTION_STATE_GET_DISCORD_USERNAME:
        // If we got the discord username, show the message box
        if (QFile::exists(DISCORD_USERNAME_FILE)) {
            actionState = ACTION_STATE_SHOW_DISCORD_MSGBOX;
            WingletGUI::inst->showMessageBox("On the next screen you will see a QR Code... "
                                             "The code contains an email template with your username and a "
                                             "verification code required to join. Please send this email out "
                                             "and you will be then added to the private discord channel. "
                                             "Note that your discord username will be tied to your badge.\n\n"
                                             "This code can be scanned with iOS Camera or Google Lens.",
                                             "Private Discord Code", "Continue");
            return;
        }
        break;

    case ACTION_STATE_SHOW_DISCORD_MSGBOX:
        // Just returned from the join discord msgbox, we're now good to quit the application to the Python script
        QApplication::exit(EXIT_CODE_REGISTER_DISCORD);
        return;

    case ACTION_STATE_WIFI_SCAN_RETURN:
    case ACTION_STATE_WIFI_SSID_RETURN: {
        // Only advance to next step if an SSID was selected
        if (enteredWifiSSID.length() > 0) {
            bool manuallyEntered = actionState == SettingsMenu::ACTION_STATE_WIFI_SSID_RETURN;

            QMap<int, QString> knownNetworks = WingletGUI::inst->wifiMon->knownNetworks();
            if (knownNetworks.key(enteredWifiSSID, -1) != -1) {
                actionState = ACTION_STATE_MSGBOX_RETURN;
                WingletGUI::inst->showMessageBox("Error: WiFi SSID is already added!\nTo change the PSK you must remove the network first.", "SSID Exists");
                return;
            }
            else if (manuallyEntered || wifiScanAskPsk) {
                actionState = ACTION_STATE_WIFI_PSK_RETURN;
                CircularKeyboard *kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
                kbd->setTitle("Join:\n" + enteredWifiSSID);
                kbd->setPasswordMaskEnable(true);
                if (manuallyEntered) {
                    kbd->setPrompt("Enter Network PSK:\n(Leave empty for open network)");
                    kbd->setAllowEmptyInput(true);
                }
                else {
                    kbd->setPrompt("Enter Network PSK:");
                    kbd->setAllowEmptyInput(false);
                }
                QValidator *validator = new WifiPSKValidator(kbd);
                kbd->setValidator(validator);
                kbd->setValidatorFailedMsg("PSK must be between 8-63 characters");
                kbd->setMaxLength(63);  // PSK max length of 63 letters
                connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
                WingletGUI::inst->addWidgetOnTop(kbd);
                return;
            }
            else {
                // In this case, this was due to a SSID scan selected on an unencrypted SSID
                // Just directly join the network
                WingletGUI::inst->wifiMon->addOpenNetwork(enteredWifiSSID);
            }
        }
        break;
    }

    case ACTION_STATE_RESET_ROOT_PASSWORD_CONFIRM:
        if (selectedMsgboxBtnIdx == 0) { // Yes button clicked
            if (!RootPasswordSetting::clearRootPassword()) {
                WingletGUI::inst->showMessageBox("Failed to clear root password!", "Password Set Error");
                actionState = ACTION_STATE_MSGBOX_RETURN;
                return;
            }
        }
        break;

    case ACTION_STATE_COLD_BOOT_GPS_CONFIRM:
        if (selectedMsgboxBtnIdx == 0) { // Yes button clicked
            int ret = system("echo -e \"\\$PCAS10,2*1E\r\n\" > /dev/gnss0");
            if (!ret) {
                WingletGUI::inst->showMessageBox("Please place badge by a window until GPS has position lock", "Almanac Cleared");
                actionState = ACTION_STATE_MSGBOX_RETURN;
                return;
            }
        }
        break;

    case ACTION_STATE_SHOW_INVALID_CANARD_SETTINGS:
        actionState = ACTION_STATE_MENU_CLOSE;
        WingletGUI::inst->showMessageBox("Failed to load settings from SAO! Unplug/plug it back in and try again.",
                                         "Settings Load Failure!", "Return");
        return;

    case ACTION_STATE_SHOW_CANARD_SETTINGS_SAVE_FAIL:
        actionState = ACTION_STATE_MENU_CLOSE;
        WingletGUI::inst->showMessageBox("Failed to save settings to device!\nReverted to previous settings.", "Failed to Save", "Okay", true);
        return;

    case ACTION_STATE_MENU_CLOSE:
        WingletGUI::inst->removeWidgetOnTop(this);
        return;

    default:
        break;
    }

    actionState = ACTION_STATE_MENU_OPEN;  // Reset state in case we get another show event for some reason

    // Bubble show event to the menu widget so we get the cool animations
    menuWidget->show();
}

void SettingsMenu::cicularKbdTextEntered(QString val)
{
    if (actionState == ACTION_STATE_GET_DISCORD_USERNAME) {
        QFile file(DISCORD_USERNAME_FILE);
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            QTextStream stream(&file);
            stream << val << Qt::endl;
        }
    }
    else if (actionState == ACTION_STATE_WIFI_SSID_RETURN) {
        enteredWifiSSID = val;
    }
    else if (actionState == ACTION_STATE_WIFI_PSK_RETURN) {
        // When the PSK is entered, we can join the SSID from the previous stage as well as the new value
        if (val.length() > 0) {
            WingletGUI::inst->wifiMon->addProtectedNetwork(enteredWifiSSID, val);
        }
        else {
            WingletGUI::inst->wifiMon->addOpenNetwork(enteredWifiSSID);
        }
    }
    else if (actionState == ACTION_STATE_TEXT_RETURN) {
        dynamic_cast<AbstractTextSetting*>(activeItem)->setValue(val);
    }
}

void SettingsMenu::selectorIndexSelected(QModelIndex index) {
    if (actionState == ACTION_STATE_SELECTOR_RETURN) {
        dynamic_cast<AbstractListSetting*>(activeItem)->setValue(index);
    }
    else if (actionState == ACTION_STATE_MANAGE_NETWORKS_RETURN) {
        QVariant actionVariant = index.data(Qt::UserRole);
        if (actionVariant.type() == QVariant::Int) {
            int actionEnum = actionVariant.toInt();
            if (actionEnum == KnownNetworksModel::FORGET_NETWORK) {
                WingletGUI::inst->wifiMon->removeNetwork(index.internalId());
            }
        }
    }
    else if (actionState == ACTION_STATE_WIFI_SCAN_RETURN) {
        QVariant selectedSsidVariant = index.data(Qt::UserRole);
        if (selectedSsidVariant.type() == QVariant::UserType) {
            WifiScanResult result = selectedSsidVariant.value<WifiScanResult>();
            enteredWifiSSID = result.ssid;
            wifiScanAskPsk = result.encrypted;
        }
    }
}

void SettingsMenu::menuExitRequested()
{
    if (isCanardSettings && WingletGUI::inst->saoMonitor->canard->settings->needsSave()) {
        auto msgbox = new MessageBox(WingletGUI::inst);
        msgbox->setMessageText("Settiings have been changed! Would you like to commit them to the device?");
        msgbox->setTitleText("Confirm Save");
        QStringList myBtns;
        myBtns << "Save";
        myBtns << "Quit";
        myBtns << "Keep Editing";
        msgbox->setButtons(myBtns);
        msgbox->setSelectedIndex(2);  // By default, show click editing
        connect(msgbox, SIGNAL(buttonClicked(int)), this, SLOT(menuExitConfirmClicked(int)));
        actionState = ACTION_STATE_MENU_OPEN;  // Reset state to initial open state
        WingletGUI::inst->addWidgetOnTop(msgbox);
    }
    else {
        WingletGUI::inst->removeWidgetOnTop(this);
    }
}

void SettingsMenu::menuExitConfirmClicked(int btnIdx) {
    if (btnIdx == 0) {
        // Save Settings
        if (!WingletGUI::inst->saoMonitor->canard->settings->saveSettings()) {
            actionState = ACTION_STATE_SHOW_CANARD_SETTINGS_SAVE_FAIL;
        }
        else {
            actionState = ACTION_STATE_MENU_CLOSE;
        }
    }
    else if (btnIdx == 1) {
        // Clear Settings
        WingletGUI::inst->saoMonitor->canard->settings->loadSettings();
        actionState = ACTION_STATE_MENU_CLOSE;
    }
    else {
        // Keep Editing, the show event will handle it
    }
}

void SettingsMenu::focusInEvent(QFocusEvent* ev)
{
    // Pass focus events to the underlying menu widget so it receives key events
    menuWidget->setFocus(ev->reason());
}

void SettingsMenu::msgboxBtnClicked(int btnIdx) {
    selectedMsgboxBtnIdx = btnIdx;
}

void SettingsMenu::menuItemSelected(QModelIndex index)
{
    auto userRole = menuWidget->model()->data(index, Qt::UserRole);
    if (userRole.isValid()) {
        auto settingsEntry = userRole.value<AbstractSettingsEntry*>();

        if (auto listEntry = dynamic_cast<AbstractListSetting*>(settingsEntry)) {
            actionState = ACTION_STATE_SELECTOR_RETURN;
            activeItem = settingsEntry;
            auto curIdx = listEntry->curIndex();
            int initialRow = 0;
            while (curIdx.isValid() && curIdx.parent().isValid()) {
                curIdx = curIdx.parent();
            }
            initialRow = curIdx.row();  // Only set initial row when parent is invalid (root node in list)

            auto selector = new SelectorBox(WingletGUI::inst, listEntry->selectionModel(), true, initialRow);
            connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
            WingletGUI::inst->addWidgetOnTop(selector);
            return;
        }
        else if (auto textEntry = dynamic_cast<AbstractTextSetting*>(settingsEntry))
        {
            actionState = ACTION_STATE_TEXT_RETURN;
            activeItem = settingsEntry;

            CircularKeyboard* kbd = new CircularKeyboard(textEntry->inputKeys(), WingletGUI::inst);
            kbd->setTitle(textEntry->title());
            kbd->setPrompt(textEntry->prompt());
            kbd->setMaxLength(textEntry->maxLength());
            QValidator *validator = textEntry->validator();
            if (validator) {
                kbd->setValidator(validator);
            }
            QString failedMsg = textEntry->validatorFailedMsg();
            if (!failedMsg.isEmpty()) {
                kbd->setValidatorFailedMsg(failedMsg);
            }
            kbd->setPasswordMaskEnable(textEntry->isPasswordField());
            kbd->setAllowEmptyInput(kbd->allowEmptyInput());
            kbd->setValue(textEntry->value());

            connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
            WingletGUI::inst->addWidgetOnTop(kbd);
            return;
        }
        else if (auto actionEntry = dynamic_cast<SettingsActionEntry*>(settingsEntry)) {
            CircularKeyboard* kbd;

            switch (actionEntry->action()) {
            case AppSettings::ACTION_ABOUT:
                actionState = ACTION_STATE_INFO_RETURN;
                WingletGUI::inst->addWidgetOnTop(new InfoViewer(WingletGUI::inst));
                return;
            case AppSettings::ACTION_RELEASE_NOTES:
                if (WingletGUI::inst->tryShowReleaseNotes(true)) {
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    return;
                }
                break;
            case AppSettings::ACTION_PRIVATE_DISCORD:
                actionState = ACTION_STATE_GET_DISCORD_USERNAME;
                QFile::remove(DISCORD_USERNAME_FILE);
                kbd = new CircularKeyboard({"abcdefghijklmnopqrstuvwxyz0123456789._"}, WingletGUI::inst);
                kbd->setTitle("Private Badge Owners Access Request");
                kbd->setPrompt("Enter Discord Username:");
                kbd->setMaxLength(32);
                kbd->setValidator(new QRegExpValidator(discord_regexp, kbd));
                kbd->setValidatorFailedMsg("Input is not a valid Discord Username");
                connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
                WingletGUI::inst->addWidgetOnTop(kbd);
                return;
            case AppSettings::ACTION_MANAGE_WIFI_NETWORKS: {
                if (WingletGUI::inst->wifiMon->wifiState() == WifiMonitor::WIFI_OFF) {
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    WingletGUI::inst->showMessageBox("Error: WiFi must be enabled in order to manage networks!", "WiFi Disabled");
                    return;
                }

                auto model = new KnownNetworksModel();
                if (model->rowCount() == 0) {
                    delete model;
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    WingletGUI::inst->showMessageBox("There are no saved wifi networks at this time.", "No Networks");
                    return;
                }
                else {
                    actionState = ACTION_STATE_MANAGE_NETWORKS_RETURN;
                    auto selector = new SelectorBox(WingletGUI::inst, model, true);
                    model->setParent(selector);
                    connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
                    WingletGUI::inst->addWidgetOnTop(selector);
                    return;
                }
            }
            case AppSettings::ACTION_WIFI_MANUAL:
                if (WingletGUI::inst->wifiMon->wifiState() == WifiMonitor::WIFI_OFF) {
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    WingletGUI::inst->showMessageBox("Error: WiFi must be enabled in order to add networks!", "WiFi Disabled");
                    return;
                }
                else {
                    enteredWifiSSID = "";  // Clear variable before entering selector
                    actionState = ACTION_STATE_WIFI_SSID_RETURN;
                    kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
                    kbd->setTitle("Manual WiFi Entry");
                    kbd->setPrompt("Enter Network SSID:");
                    kbd->setMaxLength(32);  // SSIDs have a max length of 32 bytes
                    connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
                    WingletGUI::inst->addWidgetOnTop(kbd);
                    return;
                }
            case AppSettings::ACTION_WIFI_SCAN: {
                if (WingletGUI::inst->wifiMon->wifiState() == WifiMonitor::WIFI_OFF) {
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    WingletGUI::inst->showMessageBox("Error: WiFi must be enabled in order to add networks!", "WiFi Disabled");
                    return;
                }

                enteredWifiSSID = "";  // Clear variable before entering selector
                auto model = new WifiScanModel();
                actionState = ACTION_STATE_WIFI_SCAN_RETURN;  // TODO: Replace with actual action when complete
                auto selector = new SelectorBox(WingletGUI::inst, model, true);
                selector->menuWidget->setShrinkOnSelect(false);
                model->setParent(selector);
                connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
                WingletGUI::inst->addWidgetOnTop(selector);
                return;
            }
            case AppSettings::ACTION_CLEAR_ROOT_PASSWORD: {
                actionState = ACTION_STATE_RESET_ROOT_PASSWORD_CONFIRM;
                auto msgbox = new MessageBox(WingletGUI::inst);
                msgbox->setMessageText("Are you sure you want to clear the root password? Note SSH will now only work over the USB Ethernet interface.");
                msgbox->setTitleText("Confirm Clear Password");
                QStringList myBtns;
                myBtns << "Yes";
                myBtns << "No";
                msgbox->setButtons(myBtns);
                selectedMsgboxBtnIdx = 1;  // Set default index (shouldn't happen but if we return without selecting button, we'll do default action)
                msgbox->setSelectedIndex(selectedMsgboxBtnIdx);
                connect(msgbox, SIGNAL(buttonClicked(int)), this, SLOT(msgboxBtnClicked(int)));
                WingletGUI::inst->addWidgetOnTop(msgbox);
                return;
            }
            case AppSettings::ACTION_COLD_BOOT_GPS: {
                actionState = ACTION_STATE_COLD_BOOT_GPS_CONFIRM;
                auto msgbox = new MessageBox(WingletGUI::inst);
                msgbox->setMessageText("Cold Booting the GPS will clear the satellite almanac. Reacquiring GPS position will take about 11 minutes. Are you sure?");
                msgbox->setTitleText("Cold Boot GPS?");
                QStringList myBtns;
                myBtns << "Yes";
                myBtns << "No";
                msgbox->setButtons(myBtns);
                selectedMsgboxBtnIdx = 1;  // Set default index (shouldn't happen but if we return without selecting button, we'll do default action)
                msgbox->setSelectedIndex(selectedMsgboxBtnIdx);
                connect(msgbox, SIGNAL(buttonClicked(int)), this, SLOT(msgboxBtnClicked(int)));
                WingletGUI::inst->addWidgetOnTop(msgbox);
                return;
            }
            }
        }
    }

    // If we get here, we fell through and need to show the menu again
    menuWidget->setShowShrinkFromOutside(true);
    menuWidget->show();
}

void SettingsMenu::canardConnectionChanged(bool connected) {
    // Hide settings if canard unplugged while running
    if (isCanardSettings && !connected && isVisible()) {
        WingletGUI::inst->removeWidgetOnTop(this);
    }
}

} // namespace WingletUI
