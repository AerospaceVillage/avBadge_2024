#ifndef WINGLETGUI_H
#define WINGLETGUI_H

#include <QMainWindow>
#include <QLabel>
#include <QStackedWidget>

#include "winglet-ui/hardware/gpiocontrol.h"
#include "winglet-ui/hardware/brightnesscontrol.h"
#include "winglet-ui/settings/appsettings.h"
#include "winglet-ui/worker/workerthread.h"
#include "winglet-ui/worker/wifimonitor.h"
#include "winglet-ui/worker/battmonitor.h"
#include "winglet-ui/worker/gpsreceiver.h"
#include "winglet-ui/worker/adsbreceiver.h"

class WingletGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit WingletGUI(QWidget *parent = nullptr);
    ~WingletGUI();

    void addWidgetOnTop(QWidget* widget);
    void replaceWidgetOnTop(QWidget* oldWidget, QWidget* newWidget);
    void removeWidgetOnTop(QWidget* widget);

    void showMessageBox(const QString& msg, const QString& title, const QString& buttonText = "Okay",
                        bool replaceCurrentWidget = false, Qt::Alignment alignment = Qt::AlignCenter);

    WingletUI::AppSettings settings;
    WingletUI::WifiMonitor *wifiMon;
    WingletUI::BattMonitor *battMon;
    WingletUI::GPSReceiver *gpsReceiver;
    WingletUI::ADSBReceiver *adsbReceiver;

    static WingletGUI* inst;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void gpsUpdated(WingletUI::GPSReading reading);

private:
    QStackedWidget* appStack;
#ifdef NO_HARDWARE
    QLabel *devRingLabel;
#endif

    WingletUI::WorkerThread<WingletUI::WifiMonitor> *wifiMonThread;
    WingletUI::WorkerThread<WingletUI::BattMonitor> *battMonThread;
    WingletUI::WorkerThread<WingletUI::GPSReceiver> *gpsReceiverThread;
    WingletUI::WorkerThread<WingletUI::ADSBReceiver> *adsbReceiverThread;
    WingletUI::GPIOControl *gpioControl;
    WingletUI::BrightnessControl *brightnessControl;

    QTcpSocket *dummySocket;
};

#endif // WINGLETGUI_H
