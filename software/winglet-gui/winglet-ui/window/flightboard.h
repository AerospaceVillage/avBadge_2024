#ifndef WINGLETUI_FLIGHTBOARD_H
#define WINGLETUI_FLIGHTBOARD_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QWheelEvent>
#include "winglet-ui/worker/adsbreceiver.h"
#include "winglet-ui/worker/gpsreceiver.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

class FlightBoard : public QWidget
{
    Q_OBJECT
public:
    explicit FlightBoard(QWidget *parent = nullptr);    
    ~FlightBoard();
    void keyPressEvent(QKeyEvent *ev) override;

protected:
    //void paintEvent(QPaintEvent *pEvent) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void getAirTraffic();
    void refreshTitleColors();

private:
    GPSReading currentGPS;
    QMap<quint32, Aircraft> airspace;
    QTableWidget* tableWidget;
    QStringList headers;
    QLabel* totalsLabel;

    bool doSort = false;
    bool sortDesc = true;
    int col_index_sorting = 0;

    QLabel* titleLabel;
    QLabel *avLogoLabel;
    StatusBar* statusBar;

};

} // namespace WingletUI

#endif // WINGLETUI_FLIGHTBOARD_H
