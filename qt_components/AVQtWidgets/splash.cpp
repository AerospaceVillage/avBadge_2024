#include "splash.h"

#include <QPainter>

splash::splash(QWidget *parent) :
    QWidget(parent)
//  ,  ui(new Ui::splash)
{
//    ui->setupUi(this);


}

splash::~splash()
{
//    delete ui;
}

void splash::paintEvent(QPaintEvent *pEvent){

    QPainter *painter = new QPainter(this);

    QColor *bg_color = new QColor(0,0,0);
    QBrush *bg = new QBrush(*bg_color);
    painter->setBrush(*bg);
    painter->drawRect(0,0,480,480);

    /// Show the AeroSpace Village Logo
    QRect size(240-100,240-100,200,200);
    QImage *img = new QImage(":/file/assets/alien_tesla.png");
    painter->drawImage(size,*img);
    delete bg_color;
    delete bg;
    delete img;
    delete painter;
}


