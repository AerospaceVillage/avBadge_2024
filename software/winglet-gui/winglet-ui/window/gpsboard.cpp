#include "gpsboard.h"
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include <QPainter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QScrollBar>
#include <QHeaderView>
#include <QKeyEvent>
#include "winglet-ui/worker/gpsreceiver.h"
#include "winglet-ui/hardware/ledcontrol.h"

namespace WingletUI {

#define GPS_EXPIRE_TIME_MS 30000

GPSBoard::GPSBoard(QWidget *parent)
    : QWidget{parent}
{
    // Load the last latitude/longitude readings in
    currentGPS = GPSReading(WingletGUI::inst->settings.lastLatitude(),
                            WingletGUI::inst->settings.lastLongitude(), false);

    palette.setColor(QPalette::Base, Qt::transparent);  //This is the line that makes the background transperancy work
    palette.setColor(QPalette::Text, activeTheme->palette.color(QPalette::WindowText));

    QPalette titlePalette;
    titlePalette.setColor(QPalette::WindowText, activeTheme->palette.color(QPalette::Text));
    titleLabel = new QLabel(QString("GPS Information"), this);

    //Build out the Title Label at the top & centered
    titleLabel->setFont(QFont(activeTheme->titleFont, 20));
    titleLabel->setPalette(titlePalette);
    titleLabel->setAutoFillBackground(true);

    //Center the Label
    QSize labelSize = titleLabel->minimumSizeHint();
    int x_pos = (480 - labelSize.rwidth()) / 2;
    titleLabel->setGeometry(x_pos,25,labelSize.rwidth(),30);

    posStatusLabel = new QLabel("Position Status:", this);
    posStatusLabel->setFont(QFont(activeTheme->fancyFont, 10));
    posStatusLabel->setGeometry(110,56,240,20);

    //Table with the main information
    mainTable = new QTableWidget(this);
    mainTable->setFrameShape(QFrame::NoFrame);
    mainTable->setContentsMargins(0,0,0,0);
    mainTable->setAutoFillBackground(false);
    mainTable->setBackgroundRole(QPalette::NoRole);
    mainTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    mainTable->verticalHeader()->setVisible(false);   //Turns off the row numbers
    mainTable->setRowCount(4);
    mainTable->setColumnCount(4);
    mainTable->setGeometry(80,90,280,120);
    mainTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainTable->verticalHeader()->setVisible(false);
    mainTable->horizontalHeader()->setVisible(false);
    mainTable->setPalette(palette);

    mainTable->setItem(0,0, new QTableWidgetItem("Lat"));
    mainTable->setItem(1,0, new QTableWidgetItem("Lon"));
    mainTable->setItem(2,0, new QTableWidgetItem("MSL"));
    mainTable->setItem(3,0, new QTableWidgetItem("Vel"));

    mainTable->setItem(0,2, new QTableWidgetItem("PDOP"));
    mainTable->setItem(1,2, new QTableWidgetItem("HDOP"));
    mainTable->setItem(2,2, new QTableWidgetItem("VDOP"));
    mainTable->setItem(3,2, new QTableWidgetItem("Zulu"));

    mainTable->setColumnWidth(0,60);
    mainTable->setColumnWidth(1, 80);
    mainTable->setColumnWidth(2,60);
    mainTable->setColumnWidth(3,80);


    //Display the Satellite information that is also supporting the badge
    satelliteLabel = new QLabel("Satellites Tracked", this);
    satelliteLabel->setFont(QFont(activeTheme->fancyFont, 10));
    satelliteLabel->setGeometry(100,220,250,26);

    tableWidget = new QTableWidget(this);
    tableWidget->setFrameShape(QFrame::NoFrame);
    tableWidget->setContentsMargins(0,0,0,0);
    tableWidget->setAutoFillBackground(false);
    tableWidget->setBackgroundRole(QPalette::NoRole);
    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->verticalHeader()->setVisible(false);   //Turns off the row numbers

    tableWidget->setStyleSheet("QTableWidget::item { border-left: 1px solid white; }");
    tableWidget->setPalette(palette);

    tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    tableWidget->setGeometry(100, 250, 240, 160);

    avLogoLabel = new QLabel(this);
    activeTheme->renderBgAvLogo(avLogoLabel);


    //TO DO: Determine what level of clutter we want on the display
    statusBar = new StatusBar(this);

    setGeometry(0, 0, 480, 480);

    QTimer* tir = new QTimer(this);
    getGPS();

    avLogoLabel = new QLabel(this);
    activeTheme->renderBgAvLogo(avLogoLabel);

    //// get gps structy from getData()
    connect(tir, SIGNAL(timeout()), this, SLOT(getGPS()));

    // Set LED theme.
    WingletGUI::inst->ledControl->setLedTheme(WingletGUI::inst->ledControl->gps_led_theme);
    
    // start is base on milliseconds
    tir->start(500);
}

GPSBoard::~GPSBoard()
{
    WingletGUI::inst->ledControl->setLedTheme(WingletGUI::inst->ledControl->no_led_theme);
    delete avLogoLabel;
    delete tableWidget;
    delete titleLabel;
    delete statusBar;

    delete mainTable;


}

void GPSBoard::getGPS() {

    currentGPS = WingletGUI::inst->gpsReceiver->lastReading();
    ephemeris = WingletGUI::inst->gpsReceiver->getEphemeris();
    constellation = WingletGUI::inst->gpsReceiver->getConstellation();
    auto reading = WingletGUI::inst->gpsReceiver->lastReading();
    if (reading.valid) {
        currentGPS = reading;
    }

    //qDebug() << "Constellation posStatus = " << constellation.posStatus;

    if(constellation.posStatus == GPSReceiver::PosState::POS_INVALID){
        posStatusLabel->setText(QString("Status: Acquisition"));
    }else if(constellation.posStatus == GPSReceiver::PosState::POS_2D){
        posStatusLabel->setText(QString("Status: 2D Solution"));
    }else if(constellation.posStatus == GPSReceiver::PosState::POS_3D){
        posStatusLabel->setText(QString("Status: 3D Solution"));
    }else{
        posStatusLabel->setText(QString("Status: Fault"));
    }

    satelliteLabel->setText(QString("Satellites Tracked: %1").arg(constellation.satsTracked));

    mainTable->setItem(0,1,new QTableWidgetItem(QString::number(currentGPS.latitude)));
    mainTable->setItem(1,1,new QTableWidgetItem(QString::number(currentGPS.longitude)));
    mainTable->setItem(2,1,new QTableWidgetItem(QString::number(constellation.msl)));
    mainTable->setItem(3,1,new QTableWidgetItem(QString::number(currentGPS.speedKnots)));

    mainTable->setItem(0,3,new QTableWidgetItem(QString::number(constellation.pdop)));
    mainTable->setItem(1,3,new QTableWidgetItem(QString::number(constellation.hdop)));
    mainTable->setItem(2,3,new QTableWidgetItem(QString::number(constellation.vdop)));
    mainTable->setItem(3,3,new QTableWidgetItem(currentGPS.timestamp.toString("hh:mm:ss")));

    tableWidget->clearContents();
    tableWidget->setSortingEnabled(false);
    tableWidget->setRowCount(ephemeris.size());

    tableWidget->setColumnCount(4);
    headers << "SV ID" << "Elev" << "Azim" << "CN0";
    tableWidget->setHorizontalHeaderLabels(headers);
    for(int i=0; i<4; i++){
        tableWidget->horizontalHeaderItem(i)->setFont(QFont(activeTheme->standardFont, 9));
    }
    tableWidget->setColumnWidth(0,60);
    tableWidget->setColumnWidth(1,60);
    tableWidget->setColumnWidth(2,60);
    tableWidget->setColumnWidth(3,60);

    int row = 0;
    QMapIterator<quint32, Satellite> i(ephemeris);
    while (i.hasNext()) {
        i.next();
        Satellite entry = i.value();

        QTableWidgetItem *svID = new QTableWidgetItem(QString::number(entry.svid));
        QTableWidgetItem *elev = new QTableWidgetItem(QString::number(entry.elev));
        QTableWidgetItem *azim = new QTableWidgetItem(QString::number(entry.azim));
        QTableWidgetItem *cno = new QTableWidgetItem(QString::number(entry.cno));

        svID->setFont(QFont(activeTheme->standardFont, 9));
        svID->setTextAlignment(Qt::AlignCenter);

        elev->setFont(QFont(activeTheme->standardFont, 9));
        elev->setTextAlignment(Qt::AlignCenter);

        azim->setFont(QFont(activeTheme->standardFont, 9));
        azim->setTextAlignment(Qt::AlignCenter);

        cno->setFont(QFont(activeTheme->standardFont, 9));
        cno->setTextAlignment(Qt::AlignCenter);

        // Set items in the QTableWidget
        tableWidget->setItem(row, 0, svID);
        tableWidget->setItem(row, 1, elev);
        tableWidget->setItem(row, 2, azim);
        tableWidget->setItem(row, 3, cno);

        row++;
    }

    if(doSort == true){
        tableWidget->setSortingEnabled(true);
        if(sortDesc == true){
            tableWidget->sortByColumn(col_index_sorting, Qt::DescendingOrder);
        }else{
            tableWidget->sortByColumn(col_index_sorting, Qt::AscendingOrder);
        }
    }

    WingletGUI::inst->ledControl->stepLedTheme(constellation.satsTracked);
}

void GPSBoard::keyPressEvent(QKeyEvent *ev) {
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
        doSort = !doSort;
        update();
        break;
    case Qt::Key_Left:
        col_index_sorting -= 1;
        if(col_index_sorting < 0){
            col_index_sorting = 3;
        }
        update();
        break;
    case Qt::Key_Right:
        col_index_sorting += 1;
        if(col_index_sorting >= 4){
            col_index_sorting = 0;
        }
        update();
        break;
    default:
        ev->ignore();   //Ignore and let other widgets process (ie "B" back button)
    };
}

void GPSBoard:: wheelEvent(QWheelEvent *event)  {
    QPoint numDegrees = event->angleDelta();

    if (!numDegrees.isNull()) {

        int delta = numDegrees.y() / 30;
        //qDebug() << "Going to scroll a QTableWidget: " << delta;
        tableWidget->verticalScrollBar()->setValue(tableWidget->verticalScrollBar()->value() + delta);
        tableWidget->update();
    }
    //event->accept(); // Mark the event as handled
}

} // namespace WingletUI
