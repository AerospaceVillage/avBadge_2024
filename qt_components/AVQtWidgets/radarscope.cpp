#include "radarscope.h"
#include "ui_radarscope.h"
#include "mainwindow.h"
#include <QPaintEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QPen>
#include <QtDebug>
#include <QTimer>
#include <QImageReader>
#include <QStandardPaths>

radarscope::radarscope(QWidget *parent) :
    QWidget(parent)
  ,  ui(new Ui::radarscope)
{
    ui->setupUi(this);
//    this->resize(480, 480);
//    this->setAutoFillBackground(true);
//    QPalette pal = QPalette();
//    pal.setColor(QPalette::Window, BG_COLOR);
//    this->setPalette(pal);
//    this->show();
    QTimer* tir = new QTimer(this);
    connect(tir, &QTimer::timeout, this, QOverload<>::of(&radarscope::sweep_line));

    tir->start(SWEEPINTERVAL);
    //// get aircraft structy form getData()
    connect(tir, &QTimer::timeout, this, QOverload<>::of(&radarscope::getAirTraffic));
    // start is base on milliseconds
    setGPS();

    //QHash items have to be intserted in the main method for some reason... fails when moved to the header file.
    AIRLINE_COLORS.insert(QString("SWA"), QColor(255,165,0));
    AIRLINE_COLORS.insert(QString("DAL"), QColor(0,0,255));
    AIRLINE_COLORS.insert(QString("UPS"), QColor(139,69,19));
    AIRLINE_COLORS.insert(QString("UAL"), QColor(0,0,255));
    AIRLINE_COLORS.insert(QString("AAL"), QColor(255,0,0));
    AIRLINE_COLORS.insert(QString("FDX"), QColor(162,25,255));

}

radarscope::~radarscope()
{
    delete ui;
}

/// whenever a update() is called this is executed
void radarscope::paintEvent(QPaintEvent *pEvent)
{

    // constuct painter
    QPainter *painter = new QPainter(this);    //    scene = new QGraphicsScene(this);
    if (viewMap == true){
        QPixmap* bgMap  = setMapTile(this->mapLevel, this->currentGPS.lat, this->currentGPS.lon); /// zoom has to be an int cause of the math to get XY
        qDebug()<< "Pix value: " << this->mapLevel << "avalue in the system" << int(this->pixel_to_miles+6);
        painter->drawPixmap(0,0, *bgMap);
        delete bgMap;

    }
    QBrush *bg = new QBrush(BG_COLOR);
    painter->setBrush(*bg);
    painter->drawRect(0,0,480,480);
    delete bg;

    QFont bold("Helvetica", 10); // or Fantasy or AnyStyle or Helvetica
    painter->setFont(bold);

    if (viewMap == false){

        //Draw the sweeping line, the rings and the text labels
        QPen pen(SCOPE_LINES_COLOR);
        pen.setWidth(2);
        painter->setPen(pen);
        QLine rotLine(240,240,this->line_x,this->line_y);
        painter->drawLine(rotLine);

        // set up the rings for area
        for(int x : this->ring_ranges_miles) {
            drawCirle(painter,x);
        }
        if(this->pixel_to_miles >= 15){
            drawCirle(painter,5);
        }
        if(this->pixel_to_miles <= 5){
            drawCirle(painter,100);
        }
        if(this->pixel_to_miles <= 3){
            drawCirle(painter,150);
        }
    }

    // draw Plane in the painting x, y, color
    for (int i = 0; i < airSpace.size(); i++){
        if (airSpace[i].hexAddr != QString("NONAME")){
            int infoIndex = -1;
            drawPlane(painter,airSpace[i].distance,airSpace[i].bearing, i, infoIndex);
        }
    }

    int infoIndex = -1;
    if (infoEn == true && cursorEn == true){
        infoIndex = getClosesPlane(this->cursorX,this->cursorY);
    }
    if(infoIndex != -1){
        drawPlane(painter,airSpace[infoIndex].distance,airSpace[infoIndex].bearing, infoIndex, infoIndex);
    }

    //draw cursor
    if(cursorEn == true){
        QColor cursorColor(255,255,255);
        QPen cursorPen(cursorColor);
        cursorPen.setWidth(2);
        painter->setPen(cursorPen);
        QColor cursorCenter(0,133,0);
        cursorCenter.setAlphaF( 0.0 );
        painter->setBrush(cursorCenter);
        painter->setBrush(cursorCenter);
        painter->drawEllipse(this->cursorX-10,this->cursorY-10,20,20);
        painter->drawLine(this->cursorX,this->cursorY-10,this->cursorX,this->cursorY-20);
        painter->drawLine(this->cursorX,this->cursorY+10,this->cursorX,this->cursorY+20);
        painter->drawLine(this->cursorX-20,this->cursorY,this->cursorX-10,this->cursorY);
        painter->drawLine(this->cursorX+10,this->cursorY,this->cursorX+20,this->cursorY);
    }

    //If information is attempting to be displayed render an "i" at the topif (infoEn == true /*&& cursorEn == true*/){
    if (infoEn == true /*&& cursorEn == true*/){
        QFont bold1("Cursive", 18);
        bold1.setBold(true); // or Fantasy or AnyStyle or Helvetica
        painter->setFont(bold1);
        painter->setPen(QPen(Qt::yellow));
        painter->drawText(225, 25,QString("ⓘ"));
    }

    if(painter->end()==false){
        delete painter;
        painter = NULL;
    }

}

/// draw circles for the range form the center
void radarscope::drawCirle(QPainter *paint, int val){
    float ring = 2.0*val*this->pixel_to_miles;
    int center = ((480-ring)/2);
    QPen pen(SCOPE_LINES_COLOR);
    paint->setPen(pen);
    paint->drawArc(center, center, ring, ring, 0, 16*360);
    //Draw the ring labels
    paint->drawText(231,(this->pixel_to_miles*(val)+250), QString::number(val));
}


/// rotate the line to look like scanning a radar
void radarscope::rotate(double increment ){
    this->angle += increment;
    // after 360 degress it rotates around different values
    if (this->angle >= 360){
        this->line_x = 240;
        this->line_y = -920;
        this->angle = 0;
    }
    rotate_matrix(240,240, increment); /// rotate around 240,240

}


/// do the math for scanning points location
void radarscope::rotate_matrix(double x, double y, double angle ){
    // shift to origin (0,0)
    angle = angle * (M_PI/180);
    double s = sin(angle);
    double c = cos(angle);
    this->line_x -= x;
    this->line_y -= y;

    double xr = (this->line_x * c) - (this->line_y * s) ;
    double yr = (this->line_x * s) + (this->line_y * c) ;
    this->line_x = xr + x;
    this->line_y = yr + y;

}


/// draw plane in given location x,y and color
void radarscope::drawPlane(QPainter *paint, float distance, float bearing, int aircraft_index, int infoIndex){
    if(this->airSpace[aircraft_index].distance != -1){
        xyPAIR tmp = xyPAIR();
        tmp.x = this->airSpace[aircraft_index].x_coord;
        tmp.y = this->airSpace[aircraft_index].y_coord;

        if(RENDER_AIRPLANE_IMAGE == true){
            QImage test = QImage(DEFAULT_AIRPLANE_IMAGE);
            QTransform transform = QTransform();
            transform.rotate(this->airSpace[aircraft_index].planeTrack);
            test = test.transformed(transform);
            paint->drawImage(QPoint(tmp.x-8,tmp.y-8), test);
        }else{

            QColor color = DEFAULT_AIRPLANE;

            QList<QString> keys = AIRLINE_COLORS.keys();
            foreach( auto key, keys){
                if(this->airSpace[aircraft_index].callSign->indexOf(key) == 0){
                    QHash<QString, QColor>::const_iterator i = AIRLINE_COLORS.find(key);
                    color = i.value();
                    break;
                }
            }

            if (airSpace[aircraft_index].isOnGround == true){
                color = AIRPLANE_ON_GROUND_COLOR;
            }
            paint->setBrush(QBrush(color));
            paint->setPen(QPen(color));

            QRect r1(QPoint(tmp.x-AIRCRAFT_OFFSET, tmp.y-AIRCRAFT_OFFSET),QSize(AIRCRAFT_SIZE, AIRCRAFT_SIZE));
            paint->drawEllipse(r1);
        }

        //If the cursor is hoovering over this plane then render the details of it.
        if(infoIndex != -1){
            QFont bold("Helvetica", 10); // or Fantasy or AnyStyle or Helvetica
            paint->setFont(bold);
            if(*this->airSpace[aircraft_index].callSign != QString("00000")){
                paint->drawText(tmp.x-40, tmp.y-30,QString(*this->airSpace[aircraft_index].callSign));
            }else{
                paint->drawText(tmp.x-40, tmp.y-30,QString(*this->airSpace[aircraft_index].hexAddr));
            }
            QString distance = QString::number(this->airSpace[aircraft_index].distance, 'f', 2) + "nm";
            paint->drawText(tmp.x-40, tmp.y-20, distance);
        }
    }
}


/// set the sweeping speed
void radarscope::sweep_line(){
    // rotate the defined interval step amount
    rotate(DEGREES_PER_INTERVAL);

}

/// after getting traffic form getData its inputs it in airSpace
void radarscope::setTraffic(QList<aircraft> a){
    airSpace = a;
    setGPS();
    update();
}

//// calls get data to return aircraft stuct
void radarscope::getAirTraffic(){
        emit this->callData();
}


/// set the pixel_to_miles for calcuation in getData
void radarscope::getPixToMiles()
{
 emit setPixToMiles(this->pixel_to_miles);
}

/// curser get closes plane
int radarscope::getClosesPlane(int x, int y){
    for(int i = 0; i< airSpace.size(); i++){
        if(abs(x - this->airSpace[i].x_coord)< 10 && abs(y - this->airSpace[i].y_coord) < 10){
            return i;
        }
    }
    return -1;
}



void radarscope::scopeGPS(gpsCord localGPS){
    this->currentGPS = localGPS;
//    qDebug() << "return value" << this->currentGPS.lat <<"    "<< this->currentGPS.lon ;
}

//// calls getGPS()
void radarscope::setGPS(){

    emit this->getGPS();

}


bool radarscope::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::Wheel){
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        if (this->viewMap == false){
            if(wheelEvent->angleDelta().y() > 0 ){
                if(this->pixel_to_miles +1.0 < maxZoom){
                    this->pixel_to_miles += 1.0f;
                    getPixToMiles();
                }
                update();
            } else if(wheelEvent->angleDelta().y() < 0){
                if(this->pixel_to_miles -1.0 > minZoom){
                    this->pixel_to_miles -= 1.0f;
                    getPixToMiles();
                }
                update();
            }
        }else {
            if(wheelEvent->angleDelta().y() > 0 ){
                if(this->mapLevel < 13 ){
                    this->mapLevel += 1;
                    this->pixel_to_miles = mapValue[mapLevel];
                    getPixToMiles();
                }
                update();
            } else if(wheelEvent->angleDelta().y() < 0){
                if(this->mapLevel  > 7){
                    this->mapLevel -= 1 ;
                    this->pixel_to_miles = mapValue[mapLevel];
                    getPixToMiles();
                }
                update();
            }
        }
    }
    if(event->type() == QEvent::KeyPress){
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch ( keyEvent->key() ) {
        case Qt::Key_Up:
            if(this->cursorY > 50 && this->cursorY <= 430){
                this->cursorY -= 5;
            }
            update();
            break;
        case Qt::Key_Down:
            if(this->cursorY >= 50 && this->cursorY < 430){
                this->cursorY +=5;
            }
            update();
            break;
        case Qt::Key_Right:
            if(this->cursorX >= 50 && this->cursorX < 430){
                this->cursorX += 5;
            }
            update();
            break;
        case Qt::Key_Left:
            if(this->cursorX > 50 && this->cursorX <= 430 ){
                this->cursorX -= 5;
            }
            update();
            break;

        case Qt::Key_Return:
            cursorEn = !cursorEn;
            update();
            break;
        case Qt::Key_A:
            infoEn = !infoEn;
            update();
            break;
        case Qt::Key_S:
            changeMap();
            if (this->viewMap == false){
                this->pixel_to_miles = (480.0 / (max_rannge_miles * 2));
            } else {
                this->pixel_to_miles = mapValue[mapLevel];
            }
            getPixToMiles();
            update();
            break;
        case Qt::Key_I:
            RENDER_AIRPLANE_IMAGE = !RENDER_AIRPLANE_IMAGE;
            break;
        default:
            event->ignore();
            break;
        };
    }
    return QObject::eventFilter(obj, event);
}



void radarscope::changeMap(){
   viewMap = !viewMap;

}





QPixmap* radarscope::setMapTile(double zoom, double lat, double lon){

    double n = pow(2,zoom);
    double lat_rad = lat*M_PI/180.0;;
    double x = (lon + 180.0) / 360.0 * n;
    double y = (1.0 - asinh(tan(lat_rad)) / M_PI) / 2.0 * n;

    int zo = zoom;
    double x_shift = 0;
    double y_shift = 0;

    double pixOfImage = 256; // 256 or 512
//    double pixOfImage = 512; // 256 or 512

    double correction;
    if(pixOfImage == 256){
        correction = 16;
    }
    if(pixOfImage == 512){
        correction = 272;
    }


   double stile = 40075016.686 * (cos(lat)/pow(2,zo));
   double spix = stile/256;

    x_shift = modf(x,&x);
    y_shift = modf(y,&y);
    x_shift = pixOfImage - x_shift*pixOfImage - correction; /// TODO: fix the mapping accuacy
    y_shift = (-1*y_shift*pixOfImage)- correction;

    qDebug() << "yshift: " << y_shift <<" x_shift: "<< x_shift << "correction: " << correction;
//    String fileName = QStringLiteral("/home/defcon/Downloads/mbutil/test2/%1/%2/%3.pbf").arg().arg().arg();
    QString *fileTopLeft = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y-1)); // x-1 y-1
    QString *fileTopCen = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y-1)); // y-1
    QString *fileTopRight = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y-1)); // x+1 y-1

    QString *fileCenLeft = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y)); // x-1
    QString *fileCenCen = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y)); // Replace with your PBF file path
    QString *fileCenRight = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y)); // x+1

    QString *fileBotLeft = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y+1)); // x-1 y+1
    QString *fileBotCen = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y+1)); // y+1
    QString *fileBotRight = new QString(QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y+1)); // x+1 y+1

//    qDebug() << QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y);
//    qDebug() << "path Locaiton" << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    QString zoomLength = QString::number(zo); // Set the appropriate zoom level



    QImage *rawTopLeft = new QImage(*fileTopLeft,zoomLength.toLatin1());
    QImage *rawTopCen = new QImage(*fileTopCen,zoomLength.toLatin1());
    QImage *rawTopRight = new QImage(*fileTopRight,zoomLength.toLatin1());

    QImage *rawCenLeft = new QImage(*fileCenLeft,zoomLength.toLatin1());
    QImage *rawCenCen = new QImage(*fileCenCen,zoomLength.toLatin1());
    QImage *rawCenRight = new QImage(*fileCenRight,zoomLength.toLatin1());

    QImage *rawBotLeft = new QImage(*fileBotLeft,zoomLength.toLatin1());
    QImage *rawBotCen = new QImage(*fileBotCen,zoomLength.toLatin1());
    QImage *rawBotRight = new QImage(*fileBotRight,zoomLength.toLatin1());


    int wid = pixOfImage;
    int hgt = wid;
    // Display the image in a label
    QRectF *topLeft = new QRectF((wid*-1)+x_shift,0+y_shift,wid,hgt);
    QRectF *topCenter = new QRectF(0+x_shift,0+y_shift,wid,hgt);
    QRectF *topRight = new QRectF(wid+x_shift,0+y_shift,wid,hgt);
    QRectF *centerLeft = new QRectF((wid*-1)+x_shift,hgt+y_shift,wid,hgt);
    QRectF *centerCenter = new QRectF(0+x_shift,hgt+y_shift,wid,hgt);
    QRectF *centerRight = new QRectF(wid+x_shift,hgt+y_shift,wid,hgt);
    QRectF *bottomLeft = new QRectF((wid*-1)+x_shift,hgt*2+y_shift,wid,hgt);
    QRectF *bottomCenter = new QRectF(0+x_shift,hgt*2+y_shift,wid,hgt);
    QRectF *bottomRight = new QRectF(wid+x_shift,hgt*2+y_shift,wid,hgt);

    QPixmap *pix = new QPixmap(wid*4,hgt*4); /// in reality is 480,480
    QPainter *p = new QPainter(pix);

    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*topLeft,*rawTopLeft);
    p->end();
    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*topCenter,*rawTopCen);
    p->end();
    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*topRight,*rawTopRight);
    p->end();

    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*centerLeft,*rawCenLeft);
    p->end();
    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*centerCenter,*rawCenCen);
    p->end();
    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*centerRight,*rawCenRight);
    p->end();

    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*bottomLeft,*rawBotLeft);
    p->end();
    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*bottomCenter,*rawBotCen);
    p->end();
    p->begin(pix);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*bottomRight,*rawBotRight);
    p->end();
    QColor red(255,0,0);


    delete rawTopLeft;
    delete rawTopCen;
    delete rawTopRight;
    delete rawCenLeft;
    delete rawCenCen;
    delete rawCenRight;
    delete rawBotLeft;
    delete rawBotCen;
    delete rawBotRight;

    delete topLeft;
    delete topCenter;
    delete topRight;
    delete centerLeft;
    delete centerCenter;
    delete centerRight;
    delete bottomLeft;
    delete bottomCenter;
    delete bottomRight;

    delete fileTopLeft;
    delete fileTopCen;
    delete fileTopRight;
    delete fileCenRight;
    delete fileCenCen;
    delete fileCenLeft;
    delete fileBotRight;
    delete fileBotCen;
    delete fileBotLeft;
    delete p;


    return pix;


}


