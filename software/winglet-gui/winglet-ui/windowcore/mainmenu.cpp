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
#include "winglet-ui/window/gpsboard.h"
#include "winglet-ui/window/gpstracker.h"
#include "winglet-ui/window/scrollarea.h"
#include "winglet-ui/window/radarscope.h"
#include "winglet-ui/window/mapscope.h"
#include "winglet-ui/window/settingsmenu.h"
#include "winglet-ui/window/canardboard.h"

namespace WingletUI {

enum AppType {
    APP_FLIGHT_LIST,
    APP_RADAR_SCOPE,
    APP_MAP_SCOPE,
    APP_GPS_LIST,
    APP_GPS_TRACKER,
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
    APP_CANARD,
    APP_DEBUG_RESTART_UI,
    APP_DEBUG_EXIT_FOR_HIL
};

const WingletUI::AppMenuItem demoMenuList[] = {
    {
        .title = "Media Player",
        .submenu = NULL,
        .type = APP_MEDIA_PLAYER,
    },
    {
        .title = "Clock",
        .submenu = NULL,
        .type = APP_CLOCK,
    },
    {
        .title = "GB Emulator",
        .submenu = NULL,
        .type = APP_GAMEBOY,
    },
    {
        .title = "OScope",
        .submenu = NULL,
        .type = APP_OSCOPE,
    },
    {
        .title = "GPS List",
        .submenu = NULL,
        .type = APP_GPS_LIST
    },
    {
        .title = "GPS Tracker",
        .submenu = NULL,
        .type = APP_GPS_TRACKER
    },
};

const WingletUI::AppMenuItem powerMenuList[] = {
    {
        .title = "Poweroff",
        .submenu = NULL,
        .type = APP_POWEROFF,
    },
    {
        .title = "Exit to Term",
        .submenu = NULL,
        .type = APP_EXIT_TO_TERM,
    },
    {
        .title = "Restart UI",
        .submenu = NULL,
        .type = APP_DEBUG_RESTART_UI,
    },
    {
        .title = "Debug Mode",
        .submenu = NULL,
        .type = APP_DEBUG_EXIT_FOR_HIL,
    },
    {
        .title = "Reboot",
        .submenu = NULL,
        .type = APP_REBOOT,
    },
};

const WingletUI::AppMenuItem mainMenuList[] = {
    {
        .title = "Extras",
        .submenu = demoMenuList,
        .numChildren = sizeof(demoMenuList) / sizeof(*demoMenuList),
    },
    {
        .title = "Map Scope",
        .submenu = NULL,
        .type = APP_MAP_SCOPE,
    },
    {
        .title = "Radar Scope",
        .submenu = NULL,
        .type = APP_RADAR_SCOPE,
    },
    {
        .title = "Flight List",
        .submenu = NULL,
        .type = APP_FLIGHT_LIST
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
    {
        .title = "Power",
        .submenu = powerMenuList,
        .numChildren = sizeof(powerMenuList) / sizeof(*powerMenuList),
    },
    {
        .title = "Radio Tuner",
        .submenu = NULL,
        .type = APP_CANARD,
    }
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
    // Subscribe to canard events to determine if we should add it to the main menu
    connect(WingletGUI::inst->saoMonitor->canard, SIGNAL(connectionStateChanged(bool)), this, SLOT(canardConnectionChanged(bool)));
    menuModel->disableLastMainEntry(!WingletGUI::inst->saoMonitor->canard->connected());

    // Set menu model
    menuWidget->setModel(menuModel);
    menuWidget->setCurrentIndex(2);
    connect(menuWidget, SIGNAL(itemSelected(QModelIndex)), this, SLOT(menuItemSelected(QModelIndex)));
    connect(menuWidget, SIGNAL(startingHideAnimation(unsigned int)), this, SLOT(menuBeginningHide(unsigned int)));
    connect(menuWidget, SIGNAL(startingShowAnimation(unsigned int)), this, SLOT(menuBeginningShow(unsigned int)));

    QMetaObject::connectSlotsByName(this);

    // Create AV Logo in background
    activeTheme->renderBgAvLogo(avLogoLabel);
    // Subscribe to color palette change events so the bg logo is updated if dark mode is changed
    connect(activeTheme, SIGNAL(colorPaletteChanged()), this, SLOT(colorPaletteChanged()));

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
        if (WingletGUI::inst->tryShowReleaseNotes()) {
            actionState = ACTION_STATE_RELEASE_NOTES_DONE;
            // We were able to show release notes- return here so the message box can show
            return;
        }
        break;

    case ACTION_STATE_RELEASE_NOTES_DONE:
        // User saw release notes, it's safe to clear the flag file
        QFile::remove(RELEASE_NOTES_FLAG_FILE);
        // Break and fall through to normal menu show
        break;

    case ACTION_STATE_GAMEBOY_START:
        QApplication::exit(EXIT_CODE_GAMEBOY);
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

void MainMenu::colorPaletteChanged()
{
    // Rerender the AV badge logo to update palette
    activeTheme->renderBgAvLogo(avLogoLabel);
}

void MainMenu::focusInEvent(QFocusEvent* ev)
{
    // Pass focus events to the underlying menu widget so it receives key events
    menuWidget->setFocus(ev->reason());
}

void MainMenu::canardConnectionChanged(bool connected) {
    menuModel->disableLastMainEntry(!connected);
    if (connected && isVisible() && !menuWidget->currentEntry().parent().isValid()) {
        // If we're connected, currently on the screen, and we're also not in a submenu
        // Set Canard to the current item on the cursor (the last item we just enabled)
        menuWidget->setCurrentIndex(menuModel->rowCount() - 1);
    }
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
        case APP_DEBUG_RESTART_UI:
            QApplication::exit(EXIT_CODE_RESTART_UI);
            break;
        case APP_DEBUG_EXIT_FOR_HIL:
            QApplication::exit(EXIT_CODE_HIL);
            break;
        case APP_GAMEBOY:
            actionState = ACTION_STATE_GAMEBOY_START;
            WingletGUI::inst->showMessageBox("Controls are what you'd expect, but knob is start & power button is select.\nTo exit emulator, press both knob and power buttons.", "Starting Emulator");
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
            //WingletGUI::inst->addWidgetOnTop(new ScrollArea(WingletGUI::inst));
            break;
        case APP_RADAR_SCOPE:
            WingletGUI::inst->addWidgetOnTop(new RadarScope(WingletGUI::inst));
            break;
        case APP_MAP_SCOPE:
            WingletGUI::inst->addWidgetOnTop(new MapScope(WingletGUI::inst));
            break;
        case APP_GPS_LIST:
            WingletGUI::inst->addWidgetOnTop(new GPSBoard(WingletGUI::inst));
            break;
        case APP_GPS_TRACKER:
            WingletGUI::inst->addWidgetOnTop(new GPSTracker(WingletGUI::inst));
            break;
        case APP_SETTINGS:
            WingletGUI::inst->addWidgetOnTop(new SettingsMenu(WingletGUI::inst));
            break;
        case APP_CANARD:
            // Before opening canard, set sao in control mode
            if (!WingletGUI::inst->saoMonitor->canard->setBadgeControlMode(true)) {
                WingletGUI::inst->showMessageBox("Failed to connect to board\nTry replugging the device if this issue persists.", "Failed to Connect", "Return");
            }
            else {
                WingletGUI::inst->addWidgetOnTop(new CanardBoard(WingletGUI::inst));
            }
            break;
        default:
            menuWidget->show();
            break;
        }
    }
}

} // namespace WingletUI
