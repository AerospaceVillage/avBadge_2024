#include "mainmenu.h"
#include "winglet-ui/theme.h"
#include "wingletgui.h"

#include <QPainter>
#include <QWheelEvent>
#include <QGraphicsOpacityEffect>

#include "winglet-ui/window/clock.h"
#include "winglet-ui/window/compass.h"
#include "winglet-ui/window/oscope.h"
#include "winglet-ui/window/simplemediaplayer.h"
#include "winglet-ui/window/credits.h"
#include "winglet-ui/window/flightboard.h"
#include "winglet-ui/window/radarscope.h"
#include "winglet-ui/window/settingsmenu.h"

namespace WingletUI {

enum AppType {
    APP_FLIGHT_LIST,
    APP_RADAR_SCOPE,
    APP_COMPASS,
    APP_CLOCK,
    APP_OSCOPE,
    APP_MEDIA_PLAYER,
    APP_CREDITS,
    APP_SETTINGS,
    APP_GAMEBOY,
    APP_EXIT_TO_TERM,
    APP_REBOOT,
    APP_POWEROFF,

    // Debug Menu
    APP_DEBUG_DEVINFO,
    APP_DEBUG_GPS,
    APP_DEBUG_BATTERY,
    APP_DEBUG_LEDS,
    APP_DEBUG_ADSB,
    APP_DEBUG_RESTART_UI,
    APP_DEBUG_EXIT_FOR_HIL,
};

/*
const WingletUI::AppMenuItem debugMenuList[] = {
    {
        .title = "Device Info",
        .submenu = NULL,
        .type = APP_DEBUG_DEVINFO
    },
    {
        .title = "GPS Debug",
        .submenu = NULL,
        .type = APP_DEBUG_GPS
    },
    {
        .title = "Battery Info",
        .submenu = NULL,
        .type = APP_DEBUG_BATTERY
    },
    {
        .title = "LED Debug",
        .submenu = NULL,
        .type = APP_DEBUG_LEDS
    },
    {
        .title = "ADSB Debug",
        .submenu = NULL,
        .type = APP_DEBUG_ADSB
    },
    {
        .title = "Exit 4 HIL",
        .submenu = NULL,
        .type = APP_DEBUG_EXIT_FOR_HIL
    },
    {
        .title = "Restart UI",
        .submenu = NULL,
        .type = APP_DEBUG_RESTART_UI
    }
};
*/

const WingletUI::AppMenuItem mainMenuList[] = {
    {
        .title = "Compass",
        .submenu = NULL,
        .type = APP_COMPASS,
    },
    {
        .title = "Flight List",
        .submenu = NULL,
        .type = APP_FLIGHT_LIST
    },
    {
        .title = "Radar Scope",
        .submenu = NULL,
        .type = APP_RADAR_SCOPE,
    },
    {
        .title = "Clock",
        .submenu = NULL,
        .type = APP_CLOCK,
    },
    {
        .title = "OScope",
        .submenu = NULL,
        .type = APP_OSCOPE,
    },
    {
        .title = "Media Player",
        .submenu = NULL,
        .type = APP_MEDIA_PLAYER,
    },
    {
        .title = "Credits",
        .submenu = NULL,
        .type = APP_CREDITS,
    },
    {
        .title = "Settings",
        .submenu = NULL,
        .type = APP_SETTINGS,
    },
    /*
    {
        .title = "Debug",
        .submenu = debugMenuList,
        .numChildren = sizeof(debugMenuList) / sizeof(*debugMenuList),
    },
    */
    {
        .title = "Gameboy Emu",
        .submenu = NULL,
        .type = APP_GAMEBOY,
    },
    {
        .title = "Exit to Term",
        .submenu = NULL,
        .type = APP_EXIT_TO_TERM,
    },
    {
        .title = "Reboot",
        .submenu = NULL,
        .type = APP_REBOOT,
    },
    {
        .title = "Poweroff",
        .submenu = NULL,
        .type = APP_POWEROFF,
    },
};

const WingletUI::AppMenuItem mainMenu {
    .title = "Main Menu",
    .submenu = mainMenuList,
    .numChildren = sizeof(mainMenuList) / sizeof(*mainMenuList),
};

MainMenu::MainMenu(QWidget *parent)
    : QWidget(parent), menuWidget(new ScrollableMenu(this)),
      statusBar(new StatusBar(this)),
      avLogoLabel(new QLabel(this))
{
    menuWidget->setObjectName("MenuWidget");
    menuWidget->setMenuWrap(true);
    menuWidget->setCanExitFromMenu(false);
    menuWidget->setShowShrinkFromOutside(true);
    menuWidget->setGeometry(0, 0, 480, 480);

    menuModel = new WingletUI::AppMenuModel(&mainMenu, this);
    menuWidget->setModel(menuModel);
    menuWidget->setCurrentIndex(2);
    connect(menuWidget, SIGNAL(itemSelected(QModelIndex)), this, SLOT(menuItemSelected(QModelIndex)));
    connect(menuWidget, SIGNAL(startingHideAnimation(unsigned int)), this, SLOT(menuBeginningHide(unsigned int)));
    connect(menuWidget, SIGNAL(startingShowAnimation(unsigned int)), this, SLOT(menuBeginningShow(unsigned int)));

    QMetaObject::connectSlotsByName(this);

    // Create AV Logo in background
    activeTheme->renderBgAvLogo(avLogoLabel);

    // Put status bar on top
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(statusBar);
    effect->setOpacity(1.0);
    statusBar->setGraphicsEffect(effect);

    statusBarOpacityAnimation = new QPropertyAnimation(statusBar->graphicsEffect(), "opacity", this);

    statusBar->raise();
}

MainMenu::~MainMenu()
{
    delete menuWidget;
    delete statusBar;
    delete avLogoLabel;
}

void MainMenu::showEvent(QShowEvent *event)
{
    (void) event;

    switch (actionState) {
    // Handle various states for performin actions
    case ACTION_STATE_FIRST_START:
        if (tryShowReleaseNotes())
            // We were able to show release notes- return here so the message box can show
            return;
        break;

    case ACTION_STATE_RELEASE_NOTES_DONE:
        // User saw release notes, it's safe to clear the flag file
        QFile::remove(RELEASE_NOTES_FLAG_FILE);
        // Break and fall through to normal menu show
        break;

    default:
        // If no cases above hit, just continue with showing menu
        break;
    }

    // Bubble show event to the menu widget so we get the cool animations
    menuWidget->show();
}

void MainMenu::menuBeginningHide(unsigned int duration) {
    // Signal so we can animate the status bar with the rest of the menu
    int newWindow = menuWidget->currentData().toInt();
    if (newWindow == APP_SETTINGS) {
        // The settings window has a status bar as well, so don't fade out status bar
        return;
    }

    statusBarOpacityAnimation->stop();
    statusBarOpacityAnimation->setStartValue(1);
    statusBarOpacityAnimation->setEndValue(0);
    statusBarOpacityAnimation->setDuration(duration);
    statusBarOpacityAnimation->start();
}

void MainMenu::menuBeginningShow(unsigned int duration) {
    // Signal so we can animate the status bar with the rest of the menu

    int newWindow = menuWidget->currentData().toInt();
    if (newWindow == APP_SETTINGS) {
        // The settings window has a status bar as well, so don't fade in the status bar
        return;
    }

    statusBarOpacityAnimation->stop();
    statusBarOpacityAnimation->setStartValue(0);
    statusBarOpacityAnimation->setEndValue(1);
    statusBarOpacityAnimation->setDuration(duration);
    statusBarOpacityAnimation->start();
}

bool MainMenu::tryShowReleaseNotes()
{
    // Only show release notes if flag file exiists
    if (!QFile::exists(RELEASE_NOTES_FLAG_FILE))
        return false;

    QFile file(RELEASE_NOTES_LOCATION);
    if(!file.open(QIODevice::ReadOnly)) {
        // Couldn't open release notes file
        return false;
    }

    QTextStream in(&file);
    QString title = in.readLine();
    QString body = in.readAll();
    file.close();

    if (title.isEmpty() || body.isEmpty()) {
        // Don't show empty message box
        return false;
    }

    // We've got release notes and can show them- display now
    actionState = ACTION_STATE_RELEASE_NOTES_DONE;
    WingletGUI::inst->showMessageBox(body, title, "Okay", false, Qt::AlignLeft | Qt::AlignVCenter);
    return true;
}

void MainMenu::focusInEvent(QFocusEvent* ev)
{
    // Pass focus events to the underlying menu widget so it receives key events
    menuWidget->setFocus(ev->reason());
}

void MainMenu::menuItemSelected(QModelIndex index)
{
    // Unless overidden in switch case, if we leave here, we will be returning back from a normal menu
    actionState = ACTION_STATE_RETURN;

    auto userRole = menuWidget->model()->data(index, Qt::UserRole);
    if (userRole.isValid() && userRole.type() == QVariant::Int) {
        int requestedMenu = userRole.toInt();

        switch (requestedMenu) {
        case APP_EXIT_TO_TERM:
            QApplication::exit(EXIT_CODE_RUN_TERMINAL);
            break;
        case APP_POWEROFF:
            QApplication::exit(EXIT_CODE_POWEROFF);
            break;
        case APP_REBOOT:
            QApplication::exit(EXIT_CODE_REBOOT);
            break;
        case APP_GAMEBOY:
            QApplication::exit(EXIT_CODE_GAMEBOY);
            break;
        case APP_DEBUG_RESTART_UI:
            QApplication::exit(EXIT_CODE_RESTART_UI);
            break;
        case APP_DEBUG_EXIT_FOR_HIL:
            QApplication::exit(EXIT_CODE_HIL);
            break;
        case APP_CLOCK:
            WingletGUI::inst->addWidgetOnTop(new Clock(WingletGUI::inst));
            break;
        case APP_COMPASS:
            WingletGUI::inst->addWidgetOnTop(new Compass(WingletGUI::inst));
            break;
        case APP_OSCOPE:
            WingletGUI::inst->addWidgetOnTop(new OScope(WingletGUI::inst));
            break;
        case APP_CREDITS:
            WingletGUI::inst->addWidgetOnTop(new Credits(WingletGUI::inst));
            break;
        case APP_MEDIA_PLAYER:
            WingletGUI::inst->addWidgetOnTop(new SimpleMediaPlayer(WingletGUI::inst));
            break;
        case APP_FLIGHT_LIST:
            WingletGUI::inst->addWidgetOnTop(new FlightBoard(WingletGUI::inst));
            break;
        case APP_RADAR_SCOPE:
            WingletGUI::inst->addWidgetOnTop(new RadarScope(WingletGUI::inst));
            break;
        case APP_SETTINGS:
            WingletGUI::inst->addWidgetOnTop(new SettingsMenu(WingletGUI::inst));
            break;
        default:
            menuWidget->show();
            break;
        }
    }
}

} // namespace WingletUI
