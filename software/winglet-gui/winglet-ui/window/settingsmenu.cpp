#include "settingsmenu.h"

#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include "winglet-ui/model/knownnetworksmodel.h"
#include "winglet-ui/settings/appsettingspropentry.h"
#include "winglet-ui/windowcore/circularkeyboard.h"
#include "winglet-ui/windowcore/infoviewer.h"
#include "winglet-ui/windowcore/selectorbox.h"
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

SettingsMenu::SettingsMenu(QWidget *parent)
    : QWidget{parent}, avLogoLabel(new QLabel(this)),
      statusBar(new StatusBar(this)), menuWidget(new ScrollableMenu(this))
{
    menuWidget->setMenuWrap(false);
    menuWidget->setMaxVisibleItems(9);
    menuWidget->setFontSizes(22, 14, 1);
    menuWidget->setGeometry(0, 0, 480, 480);

    menuModel = new SettingsMenuModel(WingletGUI::inst->settings.settingsEntryRoot(), this);
    menuWidget->setModel(menuModel);
    connect(menuWidget, SIGNAL(itemSelected(QModelIndex)), this, SLOT(menuItemSelected(QModelIndex)));
    connect(menuWidget, SIGNAL(menuExit()), this, SLOT(menuExitRequested()));

    // Create AV Logo in background
    activeTheme->renderBgAvLogo(avLogoLabel);

    // Status bar should appear on top
    statusBar->raise();
}

void SettingsMenu::showEvent(QShowEvent *event)
{
    (void) event;

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
            WingletGUI::inst->showMessageBox("On the next screen you will see a QR Code...\n"
                                             "The code contains an email template with your username and a\n"
                                             "verification code required to join. Please send this email out\n"
                                             "and you will be then added to the private discord channel.\n"
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

    case ACTION_STATE_WIFI_SSID_RETURN: {
        if (enteredWifiSSID.length() > 0) {
            actionState = ACTION_STATE_WIFI_PSK_RETURN;
            CircularKeyboard *kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
            kbd->setTitle("Join WiFi Network");
            kbd->setPrompt("Enter Network PSK:\n(Leave empty for open network)");
            kbd->setAllowEmptyInput(true);
            QValidator *validator = new WifiPSKValidator(kbd);
            kbd->setValidator(validator);
            kbd->setValidatorFailedMsg("PSK must be between 8-63 characters");
            kbd->setMaxLength(63);  // PSK max length of 63 letters
            connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
            WingletGUI::inst->addWidgetOnTop(kbd);
            return;
        }
    }

    default:
        break;
    }

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
}

void SettingsMenu::menuExitRequested()
{
    WingletGUI::inst->removeWidgetOnTop(this);
}

void SettingsMenu::focusInEvent(QFocusEvent* ev)
{
    // Pass focus events to the underlying menu widget so it receives key events
    menuWidget->setFocus(ev->reason());
}

void SettingsMenu::menuItemSelected(QModelIndex index)
{
    auto userRole = menuWidget->model()->data(index, Qt::UserRole);
    if (userRole.isValid()) {
        auto settingsEntry = userRole.value<AbstractSettingsEntry*>();

        if (auto listEntry = dynamic_cast<AbstractListSetting*>(settingsEntry)) {
            actionState = ACTION_STATE_SELECTOR_RETURN;
            activeItem = settingsEntry;
            auto selector = new SelectorBox(WingletGUI::inst, listEntry->selectionModel(), true, listEntry->curIndex().row());
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
                    WingletGUI::inst->showMessageBox("Error: WiFi must be enabled in order to add networks!", "WiFi Disabled");
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
            case AppSettings::ACTION_JOIN_WIFI:
                if (WingletGUI::inst->wifiMon->wifiState() == WifiMonitor::WIFI_OFF) {
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    WingletGUI::inst->showMessageBox("Error: WiFi must be enabled in order to add networks!", "WiFi Disabled");
                    return;
                }
                else {
                    actionState = ACTION_STATE_WIFI_SSID_RETURN;
                    kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
                    kbd->setTitle("Join WiFi Network");
                    kbd->setPrompt("Enter Network SSID:");
                    kbd->setMaxLength(32);  // SSIDs have a max length of 32 bytes
                    connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
                    WingletGUI::inst->addWidgetOnTop(kbd);
                    return;
                }
            }
        }
    }

    // If we get here, we fell through and need to show the menu again
    menuWidget->setShowShrinkFromOutside(true);
    menuWidget->show();
}

} // namespace WingletUI
