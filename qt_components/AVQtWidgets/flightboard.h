#ifndef FLIGHTBOARD_H
#define FLIGHTBOARD_H
#include "getdata.h"
#include "radarscope.h"

#include <QWidget>
#include <QPainter>


namespace Ui {
class flightBoard;
}


class flightBoard : public QWidget
{
    Q_OBJECT

public:
    explicit flightBoard(QWidget *parent = nullptr);
    ~flightBoard();
    QList<aircraft> airSpace;
    gpsCord currentGPS;
    QColor BG_COLOR = QColor(20,20,20);

    void getAirTraffic();
    void paintEvent(QPaintEvent *pEvent);
    void setGPS();
    int matchIndex = 0;

signals:
    void callData();
    void getGPS();

public slots:
    void setTraffic(QList<aircraft> aPlane);
    void boardGPS(gpsCord localGPS);


private:
//    Ui::flightBoard *ui;
};

#endif // FLIGHTBOARD_H
