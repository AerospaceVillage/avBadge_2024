#include "compass.h"
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QWheelEvent>
#include "wingletgui.h"

namespace WingletUI {

const static std::map<int,int> mapValue =
    {{9,11},{27,12},{45,13},{63,14},{81,15},{99,16},{117,17},
     {135,18},{153,19},{171,0},{189,1},{207,2},{225,3},{243,4},
     {261,5},{279,6},{297,7},{315,8},{333,9},{351,10},
     {-9,10},{-27,9},{-45,8},{-63,7},{-81,6},{-99,5},{-117,4},
     {-135,3},{-153,2},{-171,1},{-189,0},{-207,19},{-225,18},{-243,17},
     {-261,16},{-279,15},{-297,14},{-315,13},{-333,12},{-351,11}};

// const static rgb inColor = {.r = 1.0, .g = 0.0, .b = 0.0};
// const static rgb offColor = {};

Compass::Compass(QWidget *parent):
    QWidget{parent}
{
    label = new QLabel(this);
    label->setGeometry(0, 0, 480, 480);
}

Compass::~Compass()
{
    delete label;
}

void Compass::hideEvent(QHideEvent *ev)
{
    (void) ev;
    WingletGUI::inst->ledControl->clearRing();
}

void Compass::paintEvent(QPaintEvent *pEvent)
{
    (void) pEvent;

    QPixmap *pix = new QPixmap(480,480); /// in reality is 480,480
    QPainter *painter = new QPainter(pix);
    painter->setBrush(Qt::black);
    painter->drawRect(0,0,480,480);

    // draw backgound first
    QPoint point = QPoint(0,0);
    painter->drawImage(point, QImage(":/images/CompasPath.svg"));
    QPen pen(Qt::red);
    QBrush brush;
    painter->setBrush(Qt::SolidPattern);
    pen.setWidth(5);
    painter->setPen(pen);

    QPolygon poly;
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

    painter->translate(240,240);
    painter->rotate(b);
    painter->drawPolygon(poly);
    painter->fillPath(path,brush);
    painter->end();
    label->setPixmap(*pix);
    label->show();
    delete painter;
    delete pix;
}

void Compass::setLed(int idx, bool enable) {
    auto oldVal = mapValue.find(idx);
    if(!enable){
        WingletGUI::inst->ledControl->clearRing();
    }
    if (oldVal != mapValue.end()) {
        if(enable){
            WingletGUI::inst->ledControl->setRingLed(oldVal->second,255,0,0);
        }
        else {
            WingletGUI::inst->ledControl->setRingLed(oldVal->second,0,0,0);
        }
    }
}

void Compass::wheelEvent(QWheelEvent *ev) {

    setLed(b, false);
    b = double((int(b) + (ev->angleDelta().y() > 0 ? 9 : -9))%360);
    setLed(b, true);

    update();
}

void Compass::keyPressEvent(QKeyEvent *ev) {
    switch (ev->key()) {
    case Qt::Key::Key_Up:
    case Qt::Key::Key_Down:
        setLed(b, false);
        b = double((int(b) + (ev->key() == Qt::Key::Key_Up ? 9 : -9))%360);
        setLed(b, true);
        update();
        break;
    case Qt::Key::Key_A:
        planePointer = !planePointer;
        update();
        break;
    default:
        ev->ignore();
    }
}

} // namespace WingletUI
