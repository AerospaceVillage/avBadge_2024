#ifndef WINGLETUI_GPSBOARD_H
#define WINGLETUI_GPSBOARD_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPalette>
#include <QKeyEvent>
#include <QWheelEvent>
#include "winglet-ui/worker/gpsreceiver.h"
#include "winglet-ui/widget/statusbar.h"
#include "winglet-ui/hardware/ledcontrol.h"

namespace WingletUI {

class GPSBoard : public QWidget
{
    Q_OBJECT
public:
    explicit GPSBoard(QWidget *parent = nullptr);
    ~GPSBoard();

protected:
    //void paintEvent(QPaintEvent *pEvent) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void getGPS();

private:
    QLabel *avLogoLabel;
    GPSReading currentGPS;
    QMap<quint32, Satellite> ephemeris;
    GPSConstellation constellation;

    QTableWidget* tableWidget;
    QStringList headers;
    QPalette palette;

    QLabel* titleLabel;
    StatusBar* statusBar;

    bool doSort = false;
    bool sortDesc = true;
    int col_index_sorting = 0;

    QTableWidget* mainTable;

    QLabel *posStatusLabel;
    QLabel *satelliteLabel;
};

} // namespace WingletUI

#endif // WINGLETUI_GPSBOARD_H
