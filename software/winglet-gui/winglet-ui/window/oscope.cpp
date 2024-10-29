#include "oscope.h"
#include "rgbleds.h"
#include "winglet-ui/theme.h"

#include <QTimer>
#include <QPainter>
#include <cmath>
#include <QKeyEvent>
#include <QWheelEvent>

namespace WingletUI {

OScope::OScope(QWidget *parent)
    : QWidget{parent}
{
    this->setGeometry(0, 0, 480, 480);

    QTimer* tir = new QTimer(this);
    connect(tir, SIGNAL(timeout()), this, SLOT(moveInX()));
    tir->start(10);
}

void OScope::startLED(){
    rgb incolor;
    incolor.r = 0.0;
    incolor.g = 0.3;
    incolor.b = 0.0;

    for (int idx = 0; idx < 20; idx++) {
        rgbled_set(idx, incolor);
    }
}

void OScope::showEvent(QShowEvent *ev)
{
    (void) ev;
    startLED();
}

void OScope::hideEvent(QHideEvent *ev)
{
    (void) ev;
    rgbled_clear();
}

void OScope::paintEvent(QPaintEvent *pEvent){
    (void) pEvent;

    QPainter* painter = new QPainter(this);
    QColor bg(BG_COLOR);
    painter->setBrush(bg);
    painter->drawRect(0,0,480,480);

    //// draw Oscillosocpe grid
    QColor grid(255,255,255);
    QPen pen(grid);
    pen.setWidth(1);
    painter->setPen(pen);
    int divNum = 6;
    for(int i = 0 ; i<divNum; i++){
    /// draw verical lines
       painter->drawLine((480/divNum)*i,0,(480/divNum)*i,480);
    /// draw horizonal lines
       painter->drawLine(0,(480/divNum)*i,480,(480/divNum)*i);
    }

    QFont bold(activeTheme->standardFont, 14);
    bold.setBold(true);
    painter->setFont(bold);
    QString sta;
    QString amp("Amplitude: "+QString::number(this->amp));
    QString per("Period: "+QString::number(this->fre));

    QColor ball(255,255,0);
    if(this->state == 0){
        drawSin(painter, ball);
        sta = "WAVE: Sin";
    }
    else if(this->state == 1){
        drawAnalogSquare(painter, ball);
        sta = "WAVE: AnaSqr";
    }
    else if(this->state == 2){
        drawDigitalSquare(painter, ball);
        sta = "WAVE: DigSqr";
    }
    else if(this->state == 3){
        drawDigitalSawtooth(painter, ball);
        sta = "WAVE: Sawtooth";
    }
    else if(this->state == 4){
        drawDigitalTriangle(painter, ball);
        sta = "WAVE: Triangle";

    }
    painter->drawText(185,20, sta);
    painter->drawText(200,400, amp);
    painter->drawText(200,420, per);

    // image
    if (image == true){
        QRect size(90,90,300,300);

        QImage img(":/images/FrogFly.png");
        painter->drawImage(size,img);
    }
    delete painter;
}

void OScope::drawSin(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){
        this->points[i] = QPointF(x+float(i), (this->amp*sin(this->fre*(x+float(i))))+(upDown));
    }
    painter->drawPolyline(this->points, MAX_POINTS);
    update();
}

void OScope::drawAnalogSquare(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){

        this->points[i] = QPointF(x+float(i), (this->amp*sin(this->fre*(x+float(i))))+(upDown) + this->amp*sin(this->fre*(3*(x+float(i))))/3+ this->amp*sin(this->fre*(5*(x+float(i))))/5 + this->amp*sin(this->fre*(7*(x+float(i))))/7);
    }
    painter->drawPolyline(this->points, MAX_POINTS);
    update();
}

void OScope::drawDigitalSquare(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){

        this->points[i] = QPointF(x+float(i), 1.5*this->amp*pow(0,pow(0,(sin(this->fre*(x+float(i))))))+(upDown - 25 - this->amp/2));
    }
    painter->drawPolyline(this->points, MAX_POINTS);
    update();
}

void OScope::drawDigitalSawtooth(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){

        this->points[i] = QPointF(x+float(i), abs(fmod(this->fre*10*(x+float(i)),this->amp*1.5) - this->amp*1.5) + (upDown - 25 - this->amp/2));
    }
    painter->drawPolyline(this->points, MAX_POINTS);
    update();
}

void OScope::drawDigitalTriangle(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){

        this->points[i] = QPointF(x+float(i), (0.5)*this->amp*asin(sin(this->fre*(x+float(i)))) + (upDown));
    }
    painter->drawPolyline(this->points, MAX_POINTS);
    update();
}

void OScope::moveInX(){
    if(this->x == 480){
       this->x = -50;
    }
    this->x = this->x+1;
}

void OScope::wheelEvent(QWheelEvent *ev) {
    if(ev->angleDelta().y() > 0 ){
        if(this->state >= 0 && this->state < 4){
            this->state += 1;
        }
        update();
    }
    else {
        if(this->state > 0 && this->state <= 4){
            this->state -= 1;
        }
        update();
    }
}

void OScope::keyPressEvent(QKeyEvent *ev) {
    switch (ev->key()) {
    case Qt::Key_Up: {
        if(this->amp >= 10 && this->amp < 200){
            this->amp += 1;
        }
        update();
        break;
    }
    case Qt::Key_Down: {
        if(this->amp > 10 && this->amp <= 200){
            this->amp -=1;
        }
        update();
        break;
    }
    case Qt::Key_Right: {
        if(this->fre >= .01 && this->fre < .79){
            this->fre += .01;
        }
        update();
        break;
    }
    case Qt::Key_Left: {
        if(this->fre > .02 && this->fre <= .8 ){
            this->fre -= .01;
        }
        update();
        break;
    }
    case Qt::Key_A: {
        this->image = !this->image;
        update();
        break;
    }
    default:
        ev->ignore();
    }
}

} // namespace WingletUI
