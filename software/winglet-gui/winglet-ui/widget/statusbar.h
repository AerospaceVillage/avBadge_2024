#ifndef WINGLETUI_STATUSBAR_H
#define WINGLETUI_STATUSBAR_H

#include <QWidget>
#include <QLabel>
#include "winglet-ui/worker/wifimonitor.h"
#include "winglet-ui/worker/gpsreceiver.h"

namespace WingletUI {

class StatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);
    ~StatusBar();

protected:
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;

private slots:
    void updateWifiIcon(int state, int strength);
    void updateBatteryState(int state, int percentage);
    void updateLocationState(int state);
    void updateAdsbState(bool on);
    void timeRefreshCallback();
    void colorPaletteChanged();
    void timeFormatChanged(bool fmt12hr);

private:
    enum BattIcon {
         BATT_ICON_UNSET, BATT_CHG_0BAR, BATT_CHG_1BAR, BATT_CHG_2BAR, BATT_CHG_3BAR, BATT_CHG_4BAR,
         BATT_CHG_5BAR, BATT_CHG_6BAR, BATT_CHG_FULL, BATT_DSG_0BAR, BATT_DSG_1BAR, BATT_DSG_2BAR,
         BATT_DSG_3BAR, BATT_DSG_4BAR, BATT_DSG_5BAR, BATT_DSG_6BAR, BATT_DSG_FULL, BATT_UNKNOWN
    };
    void setBattIcon(BattIcon icon);
    void reloadPixmaps();
    void forceRefreshIcons();

    QLabel *dateTimeLabel;
    QLabel *batteryIcon;
    QLabel *locationIcon;
    QLabel *wifiIcon;
    QLabel *adsbIcon;

    BattIcon prevBattIcon = BATT_ICON_UNSET;
    QPixmap wifi_conn_0bar;
    QPixmap wifi_conn_1bar;
    QPixmap wifi_conn_2bar;
    QPixmap wifi_conn_3bar;
    QPixmap wifi_conn_4bar;
    QPixmap wifi_connecting;
    QPixmap wifi_disconnected;
    QPixmap wifi_off;

    QPixmap location_off;
    QPixmap location_nolock;
    QPixmap location_okay;

    QPixmap adsb_off;
    QPixmap adsb_on;

    QTimer *timeRefresh;
};

} // namespace WingletUI

#endif // WINGLETUI_STATUSBAR_H
