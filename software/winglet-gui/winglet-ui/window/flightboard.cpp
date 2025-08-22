#include "flightboard.h"
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include <QPainter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QScrollBar>
#include <QHeaderView>
#include <QKeyEvent>
#include <cmath>
#include <QDebug>
#include <QtMath>

namespace WingletUI {

#define GPS_EXPIRE_TIME_MS 30000

FlightBoard::FlightBoard(QWidget *parent)
    : QWidget{parent}
{
    // Load the last latitude/longitude readings in
    currentGPS = GPSReading(WingletGUI::inst->settings.lastLatitude(),
                            WingletGUI::inst->settings.lastLongitude(), false);

    QPalette tablePalette;
    tablePalette.setColor(QPalette::Base, Qt::transparent);  //This is the line that makes the background transperancy work
    tablePalette.setColor(QPalette::Text, activeTheme->palette.color(QPalette::WindowText));

    tableWidget = new QTableWidget(this);
    tableWidget->setFrameShape(QFrame::NoFrame);
    tableWidget->setContentsMargins(0,0,0,0);
    tableWidget->setAutoFillBackground(false);
    tableWidget->setBackgroundRole(QPalette::NoRole);
    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    tableWidget->setPalette(tablePalette);

    //TO DO: Decide if it's worth changing the sortIndicator arrow to something other than the arrow (ie image)
    //tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::up-arrow {image: url(':/images/alien_tesla.png'); }");


    //tableWidget->setPalette(palette);

    tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    tableWidget->setGeometry(75, 75, 310, 330);

    avLogoLabel = new QLabel(this);
    activeTheme->renderBgAvLogo(avLogoLabel);

    //Build out the Total Aircraft Label in the footer
    totalsLabel = new QLabel(this);
    totalsLabel->setGeometry(180, 440, 200, 20);
    totalsLabel->setFont(QFont(activeTheme->titleFont, 12));

    titleLabel = new QLabel(QString("Flight List"), this);
    //Build out the Title Label at the top & centered
    titleLabel->setFont(QFont(activeTheme->titleFont, 22));
    titleLabel->setForegroundRole(QPalette::Text);
    titleLabel->setAutoFillBackground(true);

    //Center the Label
    QSize labelSize = titleLabel->minimumSizeHint();
    int x_pos = (480 - labelSize.rwidth()) / 2;
    titleLabel->setGeometry(x_pos,25,labelSize.rwidth(),30);

    //TO DO: Determine what level of clutter we want on the display
    statusBar = new StatusBar(this);

    setGeometry(0, 0, 480, 480);

    QTimer* tir = new QTimer(this);
    getAirTraffic();

    //// get aircraft structy form getData()
    connect(tir, SIGNAL(timeout()), this, SLOT(getAirTraffic()));
    // start is base on milliseconds
    tir->start(100);

    //Setup LEDs
    for(int ledN = 0; ledN < WingletGUI::inst->ledControl->LED_COUNT; ledN++){
        WingletGUI::inst->ledControl->setRingLed(ledN, 0,200,0);
    }
}

FlightBoard::~FlightBoard()
{
    WingletGUI::inst->ledControl->clearRing();

    tableWidget->clear();
    delete tableWidget;
    delete titleLabel;
    delete totalsLabel;
    delete statusBar;
}

void FlightBoard::getAirTraffic() {
    airspace = WingletGUI::inst->adsbReceiver->airspace();
    auto reading = WingletGUI::inst->gpsReceiver->lastReading();
    if (reading.valid) {
        currentGPS = reading;
    }

    tableWidget->clearContents();
    tableWidget->setSortingEnabled(false);
    tableWidget->setRowCount(airspace.size());

    tableWidget->setColumnCount(5);
    headers << "Callsign" << "Alt" << "Speed" << "Hdg" << "Dist (nm)";
    tableWidget->setHorizontalHeaderLabels(headers);
    connect(activeTheme, SIGNAL(colorPaletteChanged()), this, SLOT(refreshTitleColors()));
    refreshTitleColors();
    tableWidget->setColumnWidth(0,75);
    tableWidget->setColumnWidth(1,60);
    tableWidget->setColumnWidth(2,60);
    tableWidget->setColumnWidth(3,40);
    tableWidget->setColumnWidth(4,75);

    QString totals = QString("Total Aircraft: ");
    int num_elements = airspace.size();
    QString count = QString::number(num_elements);
    QString together = totals + count;
    totalsLabel->setText(together);

    int row = 0;
    QMapIterator<quint32, Aircraft> i(airspace);
    while (i.hasNext()) {
        i.next();
        Aircraft entry = i.value();

        QTableWidgetItem* callSignItem;
        if(entry.callSignValid){
            callSignItem = new QTableWidgetItem(entry.callSign);
        }else{
            callSignItem = new QTableWidgetItem(QString::number(entry.icao24, 16).toUpper());
            callSignItem->setForeground(QBrush(activeTheme->palette.color(QPalette::Midlight)));
        }

        callSignItem->setFont(QFont(activeTheme->standardFont, 9));

        QTableWidgetItem *altItem = new QTableWidgetItem();
        if(entry.altValid){
            altItem->setData(Qt::DisplayRole, entry.alt);
        }else if(entry.onGroundValid){
            altItem->setData(Qt::DisplayRole, QString("GND"));
        }else{
            altItem->setData(Qt::DisplayRole, "-");
        }

        QTableWidgetItem* speedItem = new QTableWidgetItem();
        if(entry.gndSpeedValid){
            speedItem->setData(Qt::DisplayRole, entry.gndSpeed);
        }else{
            speedItem->setData(Qt::DisplayRole, "-");
        }

        QTableWidgetItem* hdgItem = new QTableWidgetItem();
        if(entry.planeTrackValid){
            hdgItem->setData(Qt::DisplayRole, entry.planeTrack);
        }else{
            hdgItem->setData(Qt::DisplayRole, "-");
        }

        QTableWidgetItem* distItem = new QTableWidgetItem();
        if(std::isnan(entry.distance)){
            distItem->setData(Qt::DisplayRole, "-");
        }else{
            distItem->setData(Qt::DisplayRole, qRound(entry.distance * 100.0f) / 100.0f);     //Round the number for display purposes, but keep as number
        }

        callSignItem->setFont(QFont(activeTheme->standardFont, 9));
        callSignItem->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);

        altItem->setFont(QFont(activeTheme->standardFont, 9));
        altItem->setTextAlignment(Qt::AlignCenter);

        speedItem->setFont(QFont(activeTheme->standardFont, 9));
        speedItem->setTextAlignment(Qt::AlignCenter);

        hdgItem->setFont(QFont(activeTheme->standardFont, 9));
        hdgItem->setTextAlignment(Qt::AlignCenter);

        distItem->setFont(QFont(activeTheme->standardFont, 9));
        distItem->setTextAlignment(Qt::AlignCenter);

        // Set items in the QTableWidget
        tableWidget->setItem(row, 0, callSignItem);
        tableWidget->setItem(row, 1, altItem);
        tableWidget->setItem(row, 2, speedItem);
        tableWidget->setItem(row, 3, hdgItem);
        tableWidget->setItem(row, 4, distItem);

        row++;
    }

    tableWidget->verticalHeader()->setVisible(false);

    if(doSort == true){
        tableWidget->setSortingEnabled(true);
        if(sortDesc == true){
            tableWidget->sortByColumn(col_index_sorting, Qt::DescendingOrder);            
        }else{
            tableWidget->sortByColumn(col_index_sorting, Qt::AscendingOrder);
        }
    }
}


void FlightBoard::keyPressEvent(QKeyEvent *ev) {
    switch ( ev->key() ) {
    case Qt::Key_Up:
        sortDesc = false;
        update();
        break;
    case Qt::Key_Down:
        sortDesc = true;
        update();
        break;
    case Qt::Key_Return:
        //qDebug() << "return captured";
        doSort = !doSort;
        update();
        break;
    case Qt::Key_Left:
        col_index_sorting -= 1;
        if(col_index_sorting < 0){
            col_index_sorting = 4;
        }
        update();
        break;
    case Qt::Key_Right:
        col_index_sorting += 1;
        if(col_index_sorting >= 5){
            col_index_sorting = 0;
        }
        update();
        break;
    default:
        ev->ignore();   //Ignore and let other widgets process (ie "B" back button)
    };
}

void FlightBoard:: wheelEvent(QWheelEvent *event)  {
    QPoint numDegrees = event->angleDelta();

    if (!numDegrees.isNull()) {

        int delta = numDegrees.y() / 30;
        //qDebug() << "Going to scroll a QTableWidget: " << delta;
        tableWidget->verticalScrollBar()->setValue(tableWidget->verticalScrollBar()->value() + delta);
        tableWidget->update();
    }
    //event->accept(); // Mark the event as handled
}

void FlightBoard::refreshTitleColors() {
    QPalette::ColorRole textColorRole = activeTheme->inDarkMode() ? QPalette::BrightText : QPalette::WindowText;
    QColor textColor = activeTheme->palette.color(textColorRole);
    QColor bgColor = activeTheme->palette.color(QPalette::Midlight);
    for(int i=0; i<tableWidget->horizontalHeader()->count(); i++){
        tableWidget->horizontalHeaderItem(i)->setFont(QFont(activeTheme->standardFont, 9));
        tableWidget->horizontalHeaderItem(i)->setBackground(bgColor);
        tableWidget->horizontalHeaderItem(i)->setForeground(textColor);
    }
}

} // namespace WingletUI
