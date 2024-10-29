#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include "winglet-ui/windowcore/mainmenu.h"
#include "winglet-ui/windowcore/messagebox.h"
#include <QKeyEvent>

WingletGUI* WingletGUI::inst = NULL;

WingletGUI::WingletGUI(QWidget *parent)
    : QMainWindow{parent}, settings(this),
      appStack(new QStackedWidget(this)),
      gpioControl(new WingletUI::GPIOControl(this)),
      brightnessControl(new WingletUI::BrightnessControl(this))
{
    if (inst != NULL) {
        qWarning("WingletGUI::WingletGUI: Second time GUI core initialized, not setting global instance!");
    }
    else {
        inst = this;
    }

    setObjectName("WingletGUI");

    // Create a dummy socket in the main thread before any other theads spawn
    // This prevents qwarnings sometimes about a meta type being already registered
    // (since registering in the other threads causes a race condition)
    // Sort of a hack, but it works \_shrug_/
    dummySocket = new QTcpSocket(this);

    // Create Worker Threads
    wifiMonThread = new WingletUI::WorkerThread<WingletUI::WifiMonitor>();
    wifiMonThread->start();
    wifiMon = wifiMonThread->getWorkerObj();
    settings.reportWifiMonReady();

    battMonThread = new WingletUI::WorkerThread<WingletUI::BattMonitor>();
    battMonThread->start();
    battMon = battMonThread->getWorkerObj();

    qRegisterMetaType<WingletUI::GPSReading>();
    gpsReceiverThread = new WingletUI::WorkerThread<WingletUI::GPSReceiver>();
    gpsReceiverThread->start();
    gpsReceiver = gpsReceiverThread->getWorkerObj();
    connect(gpsReceiver, SIGNAL(gpsUpdated(WingletUI::GPSReading)), this, SLOT(gpsUpdated(WingletUI::GPSReading)));

    adsbReceiverThread = new WingletUI::WorkerThread<WingletUI::ADSBReceiver>();
    adsbReceiverThread->start();
    adsbReceiver = adsbReceiverThread->getWorkerObj();

    // Initialize Theme
    resize(480, 480);
    setPalette(WingletUI::activeTheme->palette);

    // Make the menu the central widget of the stack
    setCentralWidget(appStack);
    appStack->addWidget(new WingletUI::MainMenu(this));
    appStack->currentWidget()->setObjectName("MainMenu");
    appStack->setObjectName("AppStack");

    // Force dev ring to top so we can see bounds (must be last)
#ifdef NO_HARDWARE
    QPixmap ringBackground(":/images/dev_ring.png");
    devRingLabel = new QLabel(this);
    devRingLabel->setGeometry(0, 0, 480, 480);
    devRingLabel->setPixmap(ringBackground);
    devRingLabel->setFocusPolicy(Qt::NoFocus);
    devRingLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    devRingLabel->raise();
    devRingLabel->setObjectName("DevRingLabel");
#endif

    // Focus on the primary widget
    appStack->currentWidget()->setFocus();
}

WingletGUI::~WingletGUI()
{
    delete appStack;
#ifdef NO_HARDWARE
    delete devRingLabel;
#endif

    wifiMonThread->quit();
    gpsReceiverThread->quit();
    adsbReceiverThread->quit();
    battMonThread->quit();

    wifiMonThread->wait();
    delete wifiMonThread;
    battMonThread->wait();
    delete battMonThread;
    gpsReceiverThread->wait();
    delete gpsReceiverThread;
    adsbReceiverThread->wait();
    delete adsbReceiverThread;

    delete gpioControl;
    delete brightnessControl;
}

void WingletGUI::keyPressEvent(QKeyEvent *ev)
{
    // Default handling for b key (if the widget doesn't handle it) to make simple widgets easier
    if (ev->key() == Qt::Key::Key_B)
    {
        QWidget* widget = appStack->currentWidget();
        if (appStack->currentIndex() != appStack->count() - 1) {
            qWarning("WingletGUI::keyPressEvent: Current widget is not at end?");
        }
        if (widget && widget->close()) {
            appStack->removeWidget(widget);
            delete widget;
        }

        // Focus the new top widget
        widget = appStack->currentWidget();
        if (widget) {
            widget->setFocus();
        }
    }
}

void WingletGUI::addWidgetOnTop(QWidget* widget)
{
    if (appStack->indexOf(widget) >= 0) {
        qWarning("WingletGUI::addWidgetOnTop: Cannot add widget that is already added\n");
        return;
    }

    appStack->addWidget(widget);
    appStack->setCurrentWidget(widget);
    widget->setFocus();
}

void WingletGUI::replaceWidgetOnTop(QWidget* oldWidget, QWidget* newWidget)
{
    if (appStack->indexOf(newWidget) >= 0) {
        qWarning("WingletGUI::replaceWidgetOnTop: Cannot add widget that is already added\n");
        return;
    }

    if (appStack->currentWidget() != oldWidget || appStack->currentIndex() != appStack->count() - 1)
    {
        qWarning("WingletGUI::replaceWidgetOnTop: Widget provided is not top widget... Not removing");
        return;
    }

    // Set new widget to top
    appStack->addWidget(newWidget);
    appStack->setCurrentWidget(newWidget);
    newWidget->setFocus();

    // Remove old widget
    appStack->removeWidget(oldWidget);
    oldWidget->setParent(this);
    oldWidget->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);  // Let Qt handle cleanup (can't delete here in case we're called from oldWidget)
    oldWidget->close();
}

void WingletGUI::removeWidgetOnTop(QWidget* widget)
{
    if (appStack->currentWidget() != widget || appStack->currentIndex() != appStack->count() - 1)
    {
        qWarning("WingletGUI::removeWidgetOnTop: Widget provided is not top widget... Not removing");
        return;
    }

    appStack->removeWidget(widget);
    widget->setParent(this);
    widget->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);    // Let Qt handle cleanup (can't delete here in case we're called from widget)
    widget->close();

    // Switch over to the new top widget
    appStack->setCurrentIndex(appStack->count() - 1);
    if (appStack->count() > 0) {
        QWidget* newTop = appStack->widget(appStack->count() - 1);
        if (newTop) {
            newTop->setFocus();
        }
    }
}

void WingletGUI::showMessageBox(const QString& msg, const QString& title, const QString& buttonText, bool replaceCurrentWidget, Qt::Alignment alignment) {
    auto msgbox = new WingletUI::MessageBox(this, alignment);
    msgbox->setMessageText(msg);
    msgbox->setTitleText(title);
    msgbox->setButtonText(buttonText);
    if (replaceCurrentWidget) {
        replaceWidgetOnTop(appStack->currentWidget(), msgbox);
    }
    else {
        addWidgetOnTop(msgbox);
    }
}

void WingletGUI::gpsUpdated(WingletUI::GPSReading reading) {
    settings.reportGPSReading(&reading);
}

bool WingletGUI::eventFilter(QObject *object, QEvent *event)
{
    (void) object;

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key::Key_PowerOff && keyEvent->isAutoRepeat()) {
            // If power key held down (autorepeat), then shut down the device
            QApplication::exit(WingletUI::EXIT_CODE_POWEROFF);
            return true;
        }
    }
    return false;
}
