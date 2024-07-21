#include "oscope.h"
// #include "ui_oscope.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <cmath>
#include <QDebug>



oScope::oScope(QWidget *parent) :
    QWidget(parent)
    // ,     ui(new Ui::oScope)
{
    // ui->setupUi(this);
    this->resize(480, 480);
//    this->setAutoFillBackground(true);
//    QPalette pal = ui->widget->palette();
//    pal.setColor(QPalette::Window, BG_COLOR);
//    this->setPalette(pal);
//    this->show();

    QTimer* tir = new QTimer();
    connect(tir, &QTimer::timeout, this, QOverload<>::of(&oScope::moveInX));
    tir->start(10);

}

oScope::~oScope()
{
    // delete ui;
}

void oScope::paintEvent(QPaintEvent *pEvent){
    this->painter = new QPainter(this);
    QColor bg(BG_COLOR);
    this->painter->setBrush(bg);
    this->painter->drawRect(0,0,480,480);

    //// draw Oscillosocpe grid
    QColor grid(255,255,255);
    QPen pen(grid);
    pen.setWidth(1);
    this->painter->setPen(pen);
    int divNum = 6;
    for(int i = 0 ; i<divNum; i++){
    /// draw verical lines
       this->painter->drawLine((480/divNum)*i,0,(480/divNum)*i,480);
    /// draw horizonal lines
       this->painter->drawLine(0,(480/divNum)*i,480,(480/divNum)*i);
    }

    QFont bold("Verdana", 10);
    bold.setBold(true);
    this->painter->setFont(bold);
    QString* sta;
    QString amp("Amplitude: "+QString::number(this->amp));
    QString per("Period: "+QString::number(this->fre));




    QColor ball(255,255,0);
    if(this->state == 0){
        drawSin(ball);
        sta = new QString("WAVE: Sin");
    }
    else if(this->state == 1){
        drawAnalogSquare(ball);
        sta = new QString("WAVE: AnaSqr");
    }
    else if(this->state == 2){
        drawDigitalSquare(ball);
        sta = new QString("WAVE: DigSqr");
    }
    else if(this->state == 3){
        drawDigitalSawtooth(ball);
        sta = new QString("WAVE: Sawtooth");
    }
    else if(this->state == 4){
        drawDigitalTriangle(ball);
        sta = new QString("WAVE: Triangle");

    }
    this->painter->drawText(185,20, *sta);
    this->painter->drawText(200,400,amp);
    this->painter->drawText(200,420,per);

    // image
    if (image == true){
        QRect size(90,90,300,300);

        QImage img("/home/defcon/Downloads/FrogFly.png");
        this->painter->drawImage(size,img);
    }
    delete sta;
    delete this->painter;
}

void oScope::drawSin(QColor color){
    QPen pen(color);
    pen.setWidth(4);
    this->painter->setBrush(QBrush(color));
    this->painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < this->maxPoints;i++){
        this->points[i] = QPointF(x+float(i), (this->amp*sin(this->fre*(x+float(i))))+(upDown));
    }
    this->painter->drawPolyline(this->points, this->maxPoints);
    update();
}

void oScope::drawAnalogSquare(QColor color){
    QPen pen(color);
    pen.setWidth(4);
    this->painter->setBrush(QBrush(color));
    this->painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < this->maxPoints;i++){

        this->points[i] = QPointF(x+float(i), (this->amp*sin(this->fre*(x+float(i))))+(upDown) + this->amp*sin(this->fre*(3*(x+float(i))))/3+ this->amp*sin(this->fre*(5*(x+float(i))))/5 + this->amp*sin(this->fre*(7*(x+float(i))))/7);
    }
    this->painter->drawPolyline(this->points, this->maxPoints);
    update();
}

void oScope::drawDigitalSquare(QColor color){
    QPen pen(color);
    pen.setWidth(4);
    this->painter->setBrush(QBrush(color));
    this->painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < this->maxPoints;i++){

        this->points[i] = QPointF(x+float(i), 1.5*this->amp*pow(0,pow(0,(sin(this->fre*(x+float(i))))))+(upDown - 25 - this->amp/2));
    }
    this->painter->drawPolyline(this->points, this->maxPoints);
    update();
}

void oScope::drawDigitalSawtooth(QColor color){
    QPen pen(color);
    pen.setWidth(4);
    this->painter->setBrush(QBrush(color));
    this->painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < this->maxPoints;i++){

        this->points[i] = QPointF(x+float(i), abs(fmod(this->fre*10*(x+float(i)),this->amp*1.5) - this->amp*1.5) + (upDown - 25 - this->amp/2));
    }
    this->painter->drawPolyline(this->points, this->maxPoints);
    update();
}

void oScope::drawDigitalTriangle(QColor color){
    QPen pen(color);
    pen.setWidth(4);
    this->painter->setBrush(QBrush(color));
    this->painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < this->maxPoints;i++){

        this->points[i] = QPointF(x+float(i), (0.5)*this->amp*asin(sin(this->fre*(x+float(i)))) + (upDown));
    }
    this->painter->drawPolyline(this->points, this->maxPoints);
    update();
}

void oScope::moveInX(){
    if(this->x == 480){
       this->x = 0;
    }
    this->x = this->x+1;
}

void oScope::wheelEvent(QWheelEvent* event){
    if(event->angleDelta().y() > 0 ){
        if(this->state >= 0 && this->state < 4){
            this->state += 1;
        }
        update();
    } else if(event->angleDelta().y() < 0){
        if(this->state > 0 && this->state <= 4){
            this->state -= 1;
        }
        update();
    }
//    qDebug() << " value changet to " << this->fre;
}

void oScope::keyPressEvent( QKeyEvent* event ) {
//    qDebug() << "info " << b;
    switch ( event->key() ) {
    case Qt::Key_Up:
        if(this->amp >= 10 && this->amp < 200){
            this->amp += 1;
        }
        update();
        break;
    case Qt::Key_Down:
        if(this->amp > 10 && this->amp <= 200){
            this->amp -=1;
        }
        update();
        break;
    case Qt::Key_Right:
        if(this->fre >= .01 && this->fre < .79){
            this->fre += .01;
        }
        update();
        break;
    case Qt::Key_Left:
        if(this->fre > .02 && this->fre <= .8 ){
            this->fre -= .01;
        }
        update();
        break;

    case Qt::Key_A:
        this->image = !this->image;
        update();
        break;
    default:
        event->ignore();
        break;
    }
   ;

}
