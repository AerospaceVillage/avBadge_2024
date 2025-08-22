#ifndef WINGLETUI_GPSTRACKER_H
#define WINGLETUI_GPSTRACKER_H

#include <QWidget>
#include <QLabel>
#include "winglet-ui/worker/gpsreceiver.h"

#define maxZoom 40
#define minZoom 1

#define PIXEL_SIZE 10  // In Pixels
#define PIXEL_OFFSET 5 // In Pixels, want to offset this amount to render the aircaft center. If AIRCRAFT_SIZE == 1 then offset would = 0, otherwise half rounded down

namespace WingletUI {

class GPSTracker : public QWidget
{
    Q_OBJECT
public:
    explicit GPSTracker(QWidget *parent = nullptr);
    ~GPSTracker();

    bool infoStatu = false;

    int ring_ranges_miles[3] = {10, 25, 50};
    int max_rannge_miles = 55;
    int age_limit = 100;
    float pixel_to_miles = (480.0 / (max_rannge_miles * 2)); // The beginning
    int line_x = 240;
    int line_y = -920;
    double angle = 0;

    // std::map<int,int> mapValue = {{7,1.315898609975},{8,2.631814435417},{9,5.263628870835},{10,10.527257741670},{11,21.054515483339},{12,42.108480075355},{13,84.219163744832}};
    std::map<int,int> mapValue = {{7,1.51430907604252},{8,3.02863796329003},{9,6.05727592658006},{10,12.11455185316010},{11,24.22910370632020},{12,48.45757345822760},{13,96.91768276728240}};

    int mapLevel = 7;
    int matchIndex = 0;

    QList<int> pressedKeys = {0,0,0,0};
    /// cursor setup
    bool cursorEn = false;
    bool infoEn = false;
    int cursorX = 240;
    int cursorY = 240;
    bool viewMap = false;
    bool sdMap = false;
    QMap<quint32, Satellite> ephemeris;
    GPSReading currentGPS;

    /*
     * User configureable variables to set color and icon preferences.
     */
    QColor DEFAULT_AIRPLANE = QColor(255,0,255);
    QHash<QString, QColor> SAT_COLORS;

    bool RENDER_AIRPLANE_IMAGE = false;
    QString DEFAULT_AIRPLANE_IMAGE = ":/AVQtWidgets/assets/funny2.svg";


    void rotate(double increment);

//    void drawtraffic();

    void get_planePoint(double x, double y, double angle, QColor planeColor, QString name);
    void drawSat(QPainter *paint, QPoint coord, Satellite *entry);
    void drawInfoBox(QPainter *paint, QPoint coord, Satellite *entry);

    void erasePlane(int x, int y);
    void drawCirle(QPainter *paint, int val);
    
protected:
    void paintEvent(QPaintEvent *pEvent) override;
    void hideEvent(QHideEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void getEphemeris();

private:
    QLabel *label;
    QRect tempInfoBox;
};

} // namespace WingletUI

#endif // WINGLETUI_GPSTRACKER_H
