#include "gpstracker.h"
#include <QPainter>
#include <QFile>
#include <cmath>
#include "wingletgui.h"
#include "winglet-ui/theme.h"
//#include <QWheelEvent>
#include <QKeyEvent>
#include "winglet-ui/worker/gpsreceiver.h"

#define PI 3.14159265

namespace WingletUI {

GPSTracker::GPSTracker(QWidget *parent)
    : QWidget{parent}
{

    setGeometry(0, 0, 480, 480);
    label = new QLabel(this);
    label->setGeometry(0, 0, 480, 480);

    QTimer* tir = new QTimer(this);
    //// get gps structy form getData()
    connect(tir, SIGNAL(timeout()), this, SLOT(getEphemeris()));
    tir->start(1000); //Refresh rate is 1 second

    //QHash items have to be intserted in the main method for some reason... fails when moved to the header file.
    SAT_COLORS.insert(QString("GP"), QColor(0,0,255));
    SAT_COLORS.insert(QString("BD"), QColor(255,0,0));

    getEphemeris();
}

GPSTracker::~GPSTracker()
{
    delete label;
}

void GPSTracker::hideEvent(QHideEvent *ev) {
    (void) ev;
}

void GPSTracker::keyPressEvent(QKeyEvent *ev) {
    
    switch ( ev->key() ) {
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
        infoEn = !infoEn;
        update();
        infoStatu = !infoStatu;
        break;

    default:
        ev->ignore();   //Ignore and let other widgets process (ie "B" back button)
    };
}

/// whenever a update() is called this is executed
void GPSTracker::paintEvent(QPaintEvent *pEvent)
{
    (void) pEvent;

    QPixmap *pix = new QPixmap(480,480); /// in reality is 480,480
    QPainter *painter = new QPainter(pix);
    // background fade
    painter->fillRect(rect(), activeTheme->palette.color(QPalette::Window));

    QFont bold(activeTheme->standardFont, 12); // or Fantasy or AnyStyle or Helvetica
    painter->setFont(bold);

    QPen pen(*activeTheme->radar_sweep_lines_color);
    pen.setWidth(2);
    painter->setPen(pen);

    drawCirle(painter,15);
    drawCirle(painter,30);
    drawCirle(painter,45);
    drawCirle(painter,60);
    drawCirle(painter,75);

    //Crosshairs for the ephemeris view
    painter->drawLine(0,240,480,240);
    painter->drawLine(240,0,240,480);

    // draw satellite in the painting x, y, color
    bool infoAvailable = (infoEn && cursorEn);
    tempInfoBox = QRect();
    for (auto& entry : ephemeris) {

        // Compute positioning on screen
        float x_calc = entry.elev * cos((entry.azim-90)*(PI/180));
        float y_calc = entry.elev * sin((entry.azim-90)*(PI/180));
        int x_pos = int(240 + (x_calc * 2.666));
        int y_pos = int(240 + (y_calc * 2.666));
        
        QPoint coord;
        coord.setX(x_pos);
        coord.setY(y_pos);

        // Check if extra info needs to be shown
        bool showInfo = false;
        if (infoAvailable && abs(cursorX - coord.x()) < 10 && abs(cursorY - coord.y()) < 10) {
            showInfo = true;
            infoAvailable = false;
        }
        if (showInfo == true){
            drawInfoBox(painter, coord, &entry);
        }
    }

    for (auto& entry : ephemeris) {
        // Compute positioning on screen
        float x_calc = entry.elev * cos((entry.azim-90)*(PI/180));
        float y_calc = entry.elev * sin((entry.azim-90)*(PI/180));
        int x_pos = int(240 + (x_calc * 2.666));
        int y_pos = int(240 + (y_calc * 2.666));
        
        QPoint coord;
        coord.setX(x_pos);
        coord.setY(y_pos);

        // Draw the plane
        if (!tempInfoBox.contains(coord)) {
            drawSat(painter, coord, &entry);
        }
    }

    //draw cursor
    if(cursorEn == true){
        QColor cursorColor = *activeTheme->radar_sweep_cursor_color;
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
        QFont bold1(activeTheme->titleFont, 18);
        bold1.setBold(true); // or Fantasy or AnyStyle or Helvetica
        painter->setFont(bold1);
        painter->setPen(QPen(Qt::yellow));
        painter->drawText(235, 30,QString("I"));
    }
    
    label->setPixmap(*pix);
    delete painter;
    delete pix;
}

/// draw circles for the range from the center
void GPSTracker::drawCirle(QPainter *paint, int val){
    float ring = 2.0*val*2.666; // 2.666 is (480/2)/90 for elev range
    int center = ((480-ring)/2);
    //QPen pen(activeTheme->radar_sweep_lines_color);
    //paint->setPen(pen);
    paint->drawArc(center, center, ring, ring, 0, 16*360);
    //Draw the ring labels
    paint->drawText(220,(2.666*(val)+250), QString::number(val));
}

/// draw sat in given location x,y and color
void GPSTracker::drawSat(QPainter *paint, QPoint coord, Satellite *entry){
    
    QColor color;

    if (entry->azim != 0 && entry->elev != 0) {
        if (entry->svid >= 100) {
            color = QColor(255,0,0);
        } else {
            color = QColor(0,0,255);
        }

        if (entry->cno == 0) {
            color.setRgb(255,0,255); // Tracked but not in position solution
        } 
        //else {
        //    color.setAlphaF(1.0); // In position solution
        //}

        paint->setBrush(QBrush(color));
        paint->setPen(QPen(color));

        QRect r1(QPoint(coord.x()-PIXEL_OFFSET, coord.y()-PIXEL_OFFSET),QSize(PIXEL_SIZE, PIXEL_SIZE));
        paint->drawEllipse(r1);
    }
    
}
void GPSTracker::drawInfoBox(QPainter *paint, QPoint coord, Satellite *entry){
    QColor color;

    if (entry->svid >= 100) {
        color = QColor(255,0,0);
    } else {
        color = QColor(0,0,255);
    }

    QFont bold(activeTheme->standardFont, 10); // or Fantasy or AnyStyle or Helvetica
    paint->setFont(bold);

    paint->setBrush(QBrush(activeTheme->palette.color(QPalette::Window)));
    QRect box(coord.x()-75, coord.y()-15, 90, -50);
    paint->drawRect(box);
    tempInfoBox = box;
    paint->fillRect(box,QBrush(activeTheme->palette.color(QPalette::Window)));

    paint->setBrush(QBrush(color));
    
    QString track_status = "No Track";
    if (entry->cno != 0) {
        track_status = "Tracking";
    }

    QString svText = "SV " + QString::number(entry->svid) + " " + track_status;
    paint->drawText(coord.x()-70, coord.y()-50,svText);
    QString elevText = "Elev = " + QString::number(entry->elev, 'f', 2);
    paint->drawText(coord.x()-70, coord.y()-40,elevText);
    QString azimText = "Azim = " + QString::number(entry->azim, 'f', 2);
    paint->drawText(coord.x()-70, coord.y()-30,azimText);
    QString cnoText = "cn0 = " + QString::number(entry->cno, 'f', 2);
    paint->drawText(coord.x()-70, coord.y()-20,cnoText);   

}


//// calls get data to return aircraft stuct
void GPSTracker::getEphemeris(){
    ephemeris = WingletGUI::inst->gpsReceiver->getEphemeris();
   update();
} // end getEphemeris

} // namespace WingletUI
