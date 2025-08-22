#include "statusbar.h"
#include "winglet-ui/theme.h"
#include "wingletgui.h"

#include <QDateTime>

namespace WingletUI {

StatusBar::StatusBar(QWidget *parent) : QWidget{parent}
{
    setGeometry(0, 0, 480, 480);

    // Load all icons (coloring according to color palette)
    reloadPixmaps();

    // Subscribe to color palette change events so the bg logo is updated if dark mode is changed
    connect(activeTheme, SIGNAL(colorPaletteChanged()), this, SLOT(colorPaletteChanged()));

    // Subscribe to time format change so we get instant UI feel
    connect(&WingletGUI::inst->settings, SIGNAL(timeFormat12hrChanged(bool)), this, SLOT(timeFormatChanged(bool)));

    dateTimeLabel = new QLabel(this);
    dateTimeLabel->setText("Sat\nMar 14\n21:23");
    dateTimeLabel->setForegroundRole(QPalette::WindowText);
    dateTimeLabel->setAlignment(Qt::AlignCenter);
    dateTimeLabel->setFont(QFont(activeTheme->standardFont, 14));
    dateTimeLabel->setFixedSize(dateTimeLabel->sizeHint());
    moveCenter(dateTimeLabel, width() - 42, height()/2);

    // Add the icons
    batteryIcon = new QLabel(this);
    setBattIcon(BATT_UNKNOWN);
    batteryIcon->setFixedSize(batteryIcon->sizeHint());
    moveCenter(batteryIcon, width() - 50, height()/2 - 60);

    wifiIcon = new QLabel(this);
    wifiIcon->setPixmap(wifi_off);
    wifiIcon->setFixedSize(wifiIcon->sizeHint());
    moveCenter(wifiIcon, width() - 72, height()/2 - 115);

    locationIcon = new QLabel(this);
    locationIcon->setPixmap(location_off);
    locationIcon->setFixedSize(locationIcon->sizeHint());
    moveCenter(locationIcon, width() - 50, height()/2 + 70);

    adsbIcon = new QLabel(this);
    adsbIcon->setPixmap(adsb_off);
    adsbIcon->setFixedSize(adsbIcon->sizeHint());
    moveCenter(adsbIcon, width() - 72, height()/2 + 115);

    timeRefresh = new QTimer(this);
    timeRefresh->setInterval(1000);  // Refresh current time every second
    connect(timeRefresh, SIGNAL(timeout()), this, SLOT(timeRefreshCallback()));

    // Ignore all keyboard and mouse inputs
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

StatusBar::~StatusBar()
{
    delete timeRefresh;
    delete dateTimeLabel;
    delete batteryIcon;
    delete locationIcon;
    delete wifiIcon;
    delete adsbIcon;
}

void StatusBar::timeFormatChanged(bool fmt12hr) {
    (void) fmt12hr;  // Ignore, we'll directly query in time callback
    timeRefreshCallback();
}

void StatusBar::reloadPixmaps() {
    activeTheme->loadMonochromeIcon(&wifi_conn_0bar, ":/icons/wifi/conn_0bar.png");
    activeTheme->loadMonochromeIcon(&wifi_conn_1bar, ":/icons/wifi/conn_1bar.png");
    activeTheme->loadMonochromeIcon(&wifi_conn_2bar, ":/icons/wifi/conn_2bar.png");
    activeTheme->loadMonochromeIcon(&wifi_conn_3bar, ":/icons/wifi/conn_3bar.png");
    activeTheme->loadMonochromeIcon(&wifi_conn_4bar, ":/icons/wifi/conn_4bar.png");
    activeTheme->loadMonochromeIcon(&wifi_connecting, ":/icons/wifi/connecting.png");
    activeTheme->loadMonochromeIcon(&wifi_disconnected, ":/icons/wifi/disconnected.png");
    activeTheme->loadMonochromeIcon(&wifi_off, ":/icons/wifi/off.png");
    activeTheme->loadMonochromeIcon(&location_off, ":/icons/location/off.png");
    activeTheme->loadMonochromeIcon(&location_nolock, ":/icons/location/nolock.png");
    activeTheme->loadMonochromeIcon(&location_okay, ":/icons/location/okay.png");
    activeTheme->loadMonochromeIcon(&adsb_off, ":/icons/adsb/off.png");
    activeTheme->loadMonochromeIcon(&adsb_on, ":/icons/adsb/on.png");
    prevBattIcon = BATT_ICON_UNSET;
}

void StatusBar::timeRefreshCallback()
{
    auto now = QDateTime::currentDateTime();
    if (now.date().year() < 2002) {
        dateTimeLabel->setText("No\nTime");
    }
    else {
        const QString format24hr = QStringLiteral("ddd\nMMM d\nH:mm");
        const QString format12hr = QStringLiteral("ddd\nMMM d\nh:mm\nAP");

        dateTimeLabel->setText(now.toString(WingletGUI::inst->settings.timeFormat12hr() ? format12hr : format24hr));
    }
}

void StatusBar::updateWifiIcon(int state, int strength)
{
    switch (state) {
    case WifiMonitor::WIFI_CONNECTED:
        if (strength >= 80)
            wifiIcon->setPixmap(wifi_conn_4bar);
        else if (strength >= 60)
            wifiIcon->setPixmap(wifi_conn_3bar);
        else if (strength >= 40)
            wifiIcon->setPixmap(wifi_conn_2bar);
        else if (strength >= 20)
            wifiIcon->setPixmap(wifi_conn_1bar);
        else
            wifiIcon->setPixmap(wifi_conn_0bar);
        break;
    case WifiMonitor::WIFI_CONNECTING:
        wifiIcon->setPixmap(wifi_connecting);
        break;
    case WifiMonitor::WIFI_DISCONNECTED:
        wifiIcon->setPixmap(wifi_disconnected);
        break;
    case WifiMonitor::WIFI_OFF:
    default:
        wifiIcon->setPixmap(wifi_off);
        break;
    }
}

void StatusBar::updateBatteryState(int state, int percentage)
{
    if (state == BattMonitor::BATT_DISCHARGING) {
        if (percentage >= 95) {
            setBattIcon(BATT_DSG_FULL);
        }
        else if (percentage >= 85) {
            setBattIcon(BATT_DSG_6BAR);
        }
        else if (percentage >= 75) {
            setBattIcon(BATT_DSG_5BAR);
        }
        else if (percentage >= 65) {
            setBattIcon(BATT_DSG_4BAR);
        }
        else if (percentage >= 45) {
            setBattIcon(BATT_DSG_3BAR);
        }
        else if (percentage >= 25) {
            setBattIcon(BATT_DSG_2BAR);
        }
        else if (percentage >= 10) {
            setBattIcon(BATT_DSG_1BAR);
        }
        else {
            setBattIcon(BATT_DSG_0BAR);
        }
    }
    else if (state == BattMonitor::BATT_CHARGING) {
        if (percentage >= 95) {
            setBattIcon(BATT_CHG_FULL);
        }
        else if (percentage >= 85) {
            setBattIcon(BATT_CHG_6BAR);
        }
        else if (percentage >= 75) {
            setBattIcon(BATT_CHG_5BAR);
        }
        else if (percentage >= 65) {
            setBattIcon(BATT_CHG_4BAR);
        }
        else if (percentage >= 45) {
            setBattIcon(BATT_CHG_3BAR);
        }
        else if (percentage >= 25) {
            setBattIcon(BATT_CHG_2BAR);
        }
        else if (percentage >= 10) {
            setBattIcon(BATT_CHG_1BAR);
        }
        else {
            setBattIcon(BATT_CHG_1BAR);
        }
    }
    else {
        setBattIcon(BATT_UNKNOWN);
    }
}

void StatusBar::setBattIcon(BattIcon icon) {
    if (icon == prevBattIcon) {
        return;
    }

    const char* resource;
    switch (icon) {
    case BATT_CHG_0BAR:
        resource = ":/icons/batt/chg_0bar.png";
        break;
    case BATT_CHG_1BAR:
        resource = ":/icons/batt/chg_1bar.png";
        break;
    case BATT_CHG_2BAR:
        resource = ":/icons/batt/chg_2bar.png";
        break;
    case BATT_CHG_3BAR:
        resource = ":/icons/batt/chg_3bar.png";
        break;
    case BATT_CHG_4BAR:
        resource = ":/icons/batt/chg_4bar.png";
        break;
    case BATT_CHG_5BAR:
        resource = ":/icons/batt/chg_5bar.png";
        break;
    case BATT_CHG_6BAR:
        resource = ":/icons/batt/chg_6bar.png";
        break;
    case BATT_CHG_FULL:
        resource = ":/icons/batt/chg_full.png";
        break;
    case BATT_DSG_0BAR:
        resource = ":/icons/batt/dsg_0bar.png";
        break;
    case BATT_DSG_1BAR:
        resource = ":/icons/batt/dsg_1bar.png";
        break;
    case BATT_DSG_2BAR:
        resource = ":/icons/batt/dsg_2bar.png";
        break;
    case BATT_DSG_3BAR:
        resource = ":/icons/batt/dsg_3bar.png";
        break;
    case BATT_DSG_4BAR:
        resource = ":/icons/batt/dsg_4bar.png";
        break;
    case BATT_DSG_5BAR:
        resource = ":/icons/batt/dsg_5bar.png";
        break;
    case BATT_DSG_6BAR:
        resource = ":/icons/batt/dsg_6bar.png";
        break;
    case BATT_DSG_FULL:
        resource = ":/icons/batt/dsg_full.png";
        break;
    case BATT_UNKNOWN:
    default:
        resource = ":/icons/batt/unknown.png";
        break;
    }

    QPixmap pixmap;
    activeTheme->loadMonochromeIcon(&pixmap, resource);
    batteryIcon->setPixmap(pixmap);
    prevBattIcon = icon;
}

void StatusBar::updateLocationState(int state) {
    switch (state) {
    case GPSReceiver::GPS_LOCKED:
        locationIcon->setPixmap(location_okay);
        break;
    case GPSReceiver::GPS_NO_LOCK:
        locationIcon->setPixmap(location_nolock);
        break;
    case GPSReceiver::GPS_DISCONNECTED:
    default:
        locationIcon->setPixmap(location_off);
        break;
    }
}

void StatusBar::updateAdsbState(bool on)
{
    adsbIcon->setPixmap(on ? adsb_on : adsb_off);
}

void StatusBar::forceRefreshIcons() {
    updateWifiIcon(WingletGUI::inst->wifiMon->wifiState(), WingletGUI::inst->wifiMon->wifiStrength());
    updateBatteryState(WingletGUI::inst->battMon->battState(), WingletGUI::inst->battMon->percentage());
    updateLocationState(WingletGUI::inst->gpsReceiver->state());
    updateAdsbState(WingletGUI::inst->adsbReceiver->connected());
}

void StatusBar::showEvent(QShowEvent *ev)
{
    (void) ev;
    forceRefreshIcons();
    connect(WingletGUI::inst->wifiMon, SIGNAL(wifiStateChanged(int, int)), this, SLOT(updateWifiIcon(int, int)));
    connect(WingletGUI::inst->battMon, SIGNAL(battStateChanged(int, int)), this, SLOT(updateBatteryState(int, int)));
    connect(WingletGUI::inst->gpsReceiver, SIGNAL(stateUpdated(int)), this, SLOT(updateLocationState(int)));
    connect(WingletGUI::inst->adsbReceiver, SIGNAL(connectionStateChanged(bool)), this, SLOT(updateAdsbState(bool)));
    timeRefreshCallback();
    timeRefresh->start();
}

void StatusBar::hideEvent(QHideEvent *ev)
{
    (void) ev;
    disconnect(WingletGUI::inst->wifiMon, SIGNAL(wifiStateChanged(int, int)), this, SLOT(updateWifiIcon(int, int)));
    disconnect(WingletGUI::inst->battMon, SIGNAL(battStateChanged(int, int)), this, SLOT(updateBatteryState(int, int)));
    disconnect(WingletGUI::inst->gpsReceiver, SIGNAL(stateUpdated(int)), this, SLOT(updateLocationState(int)));
    disconnect(WingletGUI::inst->adsbReceiver, SIGNAL(connectionStateChanged(bool)), this, SLOT(updateAdsbState(bool)));
    timeRefresh->stop();
}

void StatusBar::colorPaletteChanged() {
    reloadPixmaps();
    forceRefreshIcons();
}

} // namespace WingletUI
