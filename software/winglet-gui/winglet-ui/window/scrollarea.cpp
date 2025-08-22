#include "scrollarea.h"
#include "wingletgui.h"

#include "winglet-ui/window/scrollarea.h"
#include "winglet-ui/window/flightboard.h"
#include "winglet-ui/theme.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QDebug>
#include <QVBoxLayout>

namespace WingletUI {

ScrollArea::ScrollArea(QWidget *parent)
    : QWidget{parent}
{
    setGeometry(0, 0, 480, 480);
    setLeds();

    //QVBoxLayout *layout = new QVBoxLayout(this);
    //scrollArea->setLayout(layout);

    QWidget *main = new QWidget(this);  //WingletGUI::inst

    scrollArea = new QScrollArea(main);

    flightBoard = new FlightBoard(scrollArea);
    //flightBoard->setAutoFillBackground(false);

    scrollArea->setGeometry(0, 0, 480, 480);
    scrollArea->setWidgetResizable(false);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setFrameShape(QFrame::NoFrame); // This is the trick to not have a white border

    QPalette scrollPalette;
    scrollPalette.setColor(QPalette::Window, Qt::transparent);
    scrollArea->setPalette(scrollPalette);

    scrollArea->setWidget(flightBoard);

    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    avLogoLabel = new QLabel(scrollArea);
    activeTheme->renderBgAvLogo(avLogoLabel);

    QPalette titlePalette;
    titlePalette.setColor(QPalette::WindowText, activeTheme->palette.color(QPalette::Text));
    titleLabel = new QLabel(QString("Flight Board"), this);

    titleLabel->setFont(QFont(activeTheme->titleFont, 20));
    titleLabel->setPalette(titlePalette);
    titleLabel->setAutoFillBackground(true);

    //Center the Label
    QSize labelSize = titleLabel->minimumSizeHint();
    int x_pos = (480 - labelSize.rwidth()) / 2;
    titleLabel->setGeometry(x_pos,25,labelSize.rwidth(),30);

    //TO DO: Determine what level of clutter we want on the display
    statusBar = new StatusBar(this);

}

ScrollArea::~ScrollArea()
{
    delete avLogoLabel;
    delete flightBoard;
    delete scrollArea;
    delete statusBar;
    WingletGUI::inst->ledControl->clearRing();
}

void ScrollArea:: wheelEvent(QWheelEvent *event)  {
    QPoint numDegrees = event->angleDelta();

    if (!numDegrees.isNull()) {

        int delta = numDegrees.y() / 30;
        //qDebug() << "Value: " << scrollArea->verticalScrollBar()->value() << " Delta: " << delta;

        if(WingletGUI::inst->settings.invertedScrollDirection()){
            scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() - delta);
        }else{
            scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() + delta);
        }
    }
    //event->accept(); // Mark the event as handled
}

void ScrollArea::keyPressEvent(QKeyEvent *ev) {
    //qDebug() << "ScrollArea: Passing keyPressEvent along";
    flightBoard->keyPressEvent(ev);
}

void ScrollArea::setLeds()
{
    for(int ledN = 0; ledN < WingletGUI::inst->ledControl->LED_COUNT; ledN++){
        WingletGUI::inst->ledControl->setRingLed(ledN, 100,100,100);
    }
}


} // namespace WingletUI
