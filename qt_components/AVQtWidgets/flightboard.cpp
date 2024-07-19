#include "flightboard.h"
#include "mainwindow.h"
#include <QTimer>


flightBoard::flightBoard(QWidget *parent) :
    QWidget(parent)\
//  ,  ui(new Ui::flightBoard)
{
//    ui->setupUi(this);

    this->resize(480, 480);
    this->setAutoFillBackground(true);
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, BG_COLOR);
    this->setPalette(pal);
    this->show();

    QTimer* tir = new QTimer(this);

    //// get aircraft structy form getData()
    connect(tir, &QTimer::timeout, this, QOverload<>::of(&flightBoard::getAirTraffic));
    // start is base on milliseconds
    tir->start(1000);
    setGPS();

}

flightBoard::~flightBoard()
{
//    delete ui;
}

/// whenever a update() is called this is executed
void flightBoard::paintEvent(QPaintEvent *pEvent)
{
    /// constuct painter
    QPainter *painter = new QPainter(this);

    QColor *bg_color = new QColor(0,139,0);
    QBrush *bg = new QBrush(*bg_color);
    painter->setBrush(*bg);
    painter->drawRect(0,0,480,480);

    /// Set up Font and Color
    QColor *fontC = new QColor(255,255,255);
    QPen *pen = new QPen(*fontC);
    QFont *bold = new QFont("Monospace", 10); // or Fantasy or AnyStyle or Helvetica
    bold->setBold(true);
    painter->setPen(*pen);
    painter->setFont(*bold);

    /// Setup display for GPS
    QRect *gps = new QRect(180,20,40,20);
    QRect *gpsLat = new QRect(220,10,150,20);
    QRect *gpsLon = new QRect(220,25,150,20);
    painter->drawText(*gps,0,QString("GPS:"));
    painter->drawText(*gpsLat,0,QString::number(this->currentGPS.lat));
    painter->drawText(*gpsLon,0,QString::number(this->currentGPS.lon));

    /// Draw table
    /// Draw the header of the table
    int spacingX = 50;
    int spacingY = 20;
    int startY = 70;
    int startX = 90;
    painter->drawText(startX-5,startY,QString("HEX"));
    painter->drawText(startX+spacingX,startY,QString("FLIGHT"));
    painter->drawText(startX+(spacingX*2+20),startY,QString("ALT"));
    painter->drawText(startX+(spacingX*3+20),startY,QString("SPEED"));
    painter->drawText(startX+(spacingX*4+20),startY,QString("LAT"));
    painter->drawText(startX+(spacingX*5+20),startY,QString("LONG"));

    /// if no planes are in the list write No Planes
    if(airSpace.size() == 0){
        QFont *bold2 = new QFont("Verdana", 30);
        bold2->setBold(false);
        painter->setFont(*bold2);
        painter->drawText(startX+10,startY+spacingY+50,QString("NO PLANES"));
        delete bold2;
    }
    startY += 10;

    /// Fill in the table
    QFont bold2("Verdana", 8);
    bold2.setBold(false);
    painter->setFont(bold2);
    for(int i = 0; i<airSpace.size();i++){
        int pasY = startY + (spacingY*i);
        QRect *hex = new QRect(startX-5,pasY,spacingX,spacingY);
        QRect *callSign = new QRect(startX+spacingX,pasY,spacingX+20,spacingY);
        QRect *alt = new QRect(startX+(spacingX*2+20),pasY,spacingX,spacingY);
        QRect *spe = new QRect(startX+(spacingX*3+20),pasY,spacingX,spacingY);
        QRect *lat = new QRect(startX+(spacingX*4+20),pasY,spacingX,spacingY);
        QRect *lon = new QRect(startX+(spacingX*5+20),pasY,spacingX,spacingY);
        painter->drawText(*hex,0,QString(*this->airSpace[i].hexAddr));
        painter->drawText(*callSign,0,QString(*this->airSpace[i].callSign));
        painter->drawText(*alt,0,QString::number(this->airSpace[i].alt));
        painter->drawText(*spe,0,QString::number(this->airSpace[i].gndSpeed));
        painter->drawText(*lat,0,QString::number(this->airSpace[i].lat));
        painter->drawText(*lon,0,QString::number(this->airSpace[i].lon));


        delete hex;
        delete callSign;
        delete alt;
        delete spe;
        delete lat;
        delete lon;

    }

    /// Show the AeroSpace Village Logo
    QRect size(5,200,80,80);
    QImage *img = new QImage(":/file/assets/AV_Logo_bad.svg");
    painter->drawImage(size,*img);

    delete bg_color;
    delete bg;
    delete fontC;
    delete pen;
    delete bold;

    delete gps;
    delete gpsLat;
    delete gpsLon;
    delete img;
    if(painter->end()==false){
        delete painter;
    }
}

/// after getAirTraffic() requests index of aircraft form getData
/// then it gives flightboard the plane
void flightBoard::setTraffic(QList<aircraft> aPlane){
    airSpace = aPlane;
    update();
}


//// calls getData requesting the plane based on index
void flightBoard::getAirTraffic(){
//    emit this->getLastPlane();
//    qDebug() << this->getdata;
    this->callData();

//    qDebug() << "calling get GPS";
    setGPS();
}

//void flightBoard::setLastPlane(int a)
//{
//    this->trafficLength = a;
//}



/// set GPS for the flightboard
/// calls getGPS to get GPS for getData
void flightBoard::setGPS(){
    // what ever we need to set current gps
    emit this->getGPS();

}


/// getData provides GPS and is set to board
void flightBoard::boardGPS(gpsCord localGPS){
    this->currentGPS = localGPS;
//    qDebug() << "return value" << this->currentGPS.lat <<"    "<< this->currentGPS.lon ;

}

