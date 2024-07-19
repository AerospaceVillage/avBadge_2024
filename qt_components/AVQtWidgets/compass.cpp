#include "compass.h"
#include "ui_compass.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QtDebug>
#include <QTimer>
#include <QImage>
#include <QPainterPath>


compass::compass(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::compass)
{
    ui->setupUi(this);
    b = 0;
}

compass::~compass()
{
    delete ui;
}

void compass::paintEvent(QPaintEvent *pEvent)
{
    QPainter *painter = new QPainter(this);

    // draw backgound first
    QPoint point = QPoint(0,0);
    painter->drawImage(point, QImage(":/file/assets/CompasPath.svg"));
    QPen pen(Qt::red);
    QBrush brush;
    painter->setBrush(Qt::SolidPattern);
    pen.setWidth(5);
    painter->setPen(pen);

    QPolygon poly;
//    poly << QPoint(220,180);
//    poly << QPoint(200,240);
//    poly << QPoint(220,220);
//    poly << QPoint(240,240);

    poly << QPoint(0,-40);
    poly << QPoint(-30,30);
    poly << QPoint(0,0);
    poly << QPoint(30,30);

    brush.setColor(Qt::red);
    brush.setStyle(Qt::SolidPattern);
    if(planePointer == true){
        QPolygon planeCompass;
        planeCompass << QPoint(0,-100);
        planeCompass << QPoint(0,-80);
        planeCompass << QPoint(10,-40);
        planeCompass << QPoint(70,0);
        planeCompass << QPoint(70,10);
        planeCompass << QPoint(10,0);
        planeCompass << QPoint(8,55);
        planeCompass << QPoint(30,70);
        planeCompass << QPoint(30,85);
        planeCompass << QPoint(0,70);
        planeCompass << QPoint(-30,85);
        planeCompass << QPoint(-30,70);
        planeCompass << QPoint(-8,55);
        planeCompass << QPoint(-10,0);
        planeCompass << QPoint(-70,10);
        planeCompass << QPoint(-70,0);
        planeCompass << QPoint(-10,-40);
        planeCompass << QPoint(0,-80);
        poly = planeCompass;
    }

    QPainterPath path;
    path.addPolygon(poly);

//    double t = 200.2;
//    Qreal rrr = new QReal(t,t);
//    QPointF trr = new QPointF(t,t);
//    painter->translate(10,10);
//    painter->rotate(0);
    painter->translate(240,240);
    painter->rotate(b);
    painter->drawPolygon(poly);
    painter->fillPath(path,brush);
    painter->end();
    delete painter;



}

void compass::keyPressEvent( QKeyEvent* event ) {
//    qDebug() << "info " << b;
    switch ( event->key() ) {
    case Qt::Key_Up:
        b = double((int(b) + 10)%360);
        update();
        break;
    case Qt::Key_Down:
        b = double((int(b) - 10)%360);
        update();
        break;
    case Qt::Key_Q:
        planePointer = !planePointer;
        update();
        break;
    case Qt::Key_M:

        break;
    default:
        event->ignore();
        break;
    }
   ;

}

void compass::wheelEvent(QWheelEvent* event){
    if(event->angleDelta().y() > 0){
        b = double((int(b) + 10)%360);
        update();
    } else if(event->angleDelta().y() < 0){
        b = double((int(b) - 10)%360);
        update();
    }
}

void compass::rotate(){

}
