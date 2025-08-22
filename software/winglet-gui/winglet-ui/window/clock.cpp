#include "clock.h"

#include <QDateTime>
#include <QTimer>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>

#include "wingletgui.h"
#include "winglet-ui/theme.h"

namespace WingletUI {

Clock::Clock(QWidget *parent)
    : QWidget{parent}, clockLabel(new QLabel(this)),
    avLogoLabel(new QLabel(this)),
    clockShowSeconds(WingletGUI::inst->settings.clockShowSeconds())
{
    clockLabel->setGeometry(0, 0, 480, 480);
    clockLabel->setAlignment(Qt::AlignCenter);
    clockLabel->setForegroundRole(QPalette::Text);
    clockLabel->setFont(QFont(activeTheme->titleFont, 40));

    // Create fade animation
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(clockLabel);
    effect->setOpacity(0.0);
    clockLabel->setGraphicsEffect(effect);

    fadeAnimation = new QPropertyAnimation(clockLabel->graphicsEffect(), "opacity", this);
    connect(fadeAnimation, SIGNAL(finished()), this, SLOT(animationFinished()));

    // Render the current time
    showTime();
    activeTheme->renderBgAvLogo(avLogoLabel);

    // Create periodic timer to update the clock
    QTimer* tir = new QTimer(this);
    connect(tir, SIGNAL(timeout()), this, SLOT(showTime()));
    tir->start(1000);

    //Setup LEDs
    for(int ledN = 0; ledN < WingletGUI::inst->ledControl->LED_COUNT; ledN++){
        WingletGUI::inst->ledControl->setRingLed(ledN, 255,172,17);
    }
}

Clock::~Clock(){
    delete clockLabel;
    delete avLogoLabel;

    WingletGUI::inst->ledControl->clearRing();
}

void Clock::showEvent(QShowEvent *ev)
{
    (void) ev;

    // Start the show animation
    fadeAnimation->stop();
    animationIsHiding = false;

    if (activeTheme->animationDuration) {
        fadeAnimation->setDuration(activeTheme->animationDuration);
        fadeAnimation->setStartValue(0.0);
        fadeAnimation->setEndValue(1.0);

        fadeAnimation->start();
    }
    else {
        clockLabel->graphicsEffect()->setProperty("opacity", 1.0);
    }
}

void Clock::hideEvent(QHideEvent *ev)
{
    (void) ev;

    // On leave, if they changed the clock show settings, apply it to settings
    if (clockShowSeconds != WingletGUI::inst->settings.clockShowSeconds())
        WingletGUI::inst->settings.setClockShowSeconds(clockShowSeconds);
}

void Clock::beginHide()
{
    // Start the show animation
    fadeAnimation->stop();
    animationIsHiding = true;

    if (activeTheme->animationDuration) {
        fadeAnimation->setDuration(activeTheme->animationDuration);
        fadeAnimation->setStartValue(1.0);
        fadeAnimation->setEndValue(0.0);

        fadeAnimation->start();
    }
    else {
        animationFinished();
    }
}

void Clock::animationFinished()
{
    if (animationIsHiding)
    {
        WingletGUI::inst->removeWidgetOnTop(this);
    }
}

void Clock::keyPressEvent(QKeyEvent *ev)
{
    // Handle return, but only if not in part of konami sequence
    if (ev->key() == Qt::Key::Key_B) {
        beginHide();
    }
    else if (ev->key() == Qt::Key::Key_A || ev->key() == Qt::Key::Key_Return) {
        clockShowSeconds = !clockShowSeconds;
        showTime();
    }
    else {
        ev->ignore();
    }
}

void Clock::showTime(){
    QString time;
    if (clockShowSeconds)
        time = QDateTime::currentDateTime().toString("hh:mm:ss");
    else
        time = QDateTime::currentDateTime().toString("hh:mm");
    clockLabel->setText(time);
}

} // namespace WingletUI
