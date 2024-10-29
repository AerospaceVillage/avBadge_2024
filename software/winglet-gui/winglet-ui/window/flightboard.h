#ifndef WINGLETUI_FLIGHTBOARD_H
#define WINGLETUI_FLIGHTBOARD_H

#include <QWidget>
#include <QLabel>
#include "winglet-ui/worker/adsbreceiver.h"
#include "winglet-ui/worker/gpsreceiver.h"

namespace WingletUI {

class FlightBoard : public QWidget
{
    Q_OBJECT
public:
    explicit FlightBoard(QWidget *parent = nullptr);
    ~FlightBoard();

protected:
    void paintEvent(QPaintEvent *pEvent) override;

private slots:
    void getAirTraffic();

private:
    QLabel *label;
    QLabel *avLogoLabel;
    GPSReading currentGPS;
    QMap<quint32, Aircraft> airspace;
};

} // namespace WingletUI

#endif // WINGLETUI_FLIGHTBOARD_H
