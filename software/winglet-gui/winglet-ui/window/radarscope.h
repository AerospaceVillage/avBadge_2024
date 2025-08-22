#ifndef WINGLETUI_RADARSCOPE_H
#define WINGLETUI_RADARSCOPE_H

#include <QWidget>
#include <QLabel>
#include <QMutex>
#include "winglet-ui/worker/adsbreceiver.h"
#include "winglet-ui/worker/gpsreceiver.h"

#define maxZoom 40
#define minZoom 1

#define SWEEPINTERVAL 50
#define DEGREES_PER_INTERVAL 9

#define AIRCRAFT_SIZE 5  // In Pixels
#define AIRCRAFT_OFFSET 2 // In Pixels, want to offset this amount to render the aircaft center. If AIRCRAFT_SIZE == 1 then offset would = 0, otherwise half rounded down

// #define LOCAL_MAP_FILES "/home/defcon2024/Downloads/hometiles"
#define LOCAL_MAP_FILES "/opt/winglet-gui/maps/"
#define SD_MAP_FILES "/mnt/sd/maps/"

namespace WingletUI {

class RadarScope : public QWidget
{
    Q_OBJECT
public:
    explicit RadarScope(QWidget *parent = nullptr);
    ~RadarScope();

    QString* mapLocation = new QString(LOCAL_MAP_FILES);

    int get_distance(float lat, float lon);
    bool infoStatu = false;

    int ring_ranges_miles[3] = {10, 25, 50};
    int max_rannge_miles = 55;
    int age_limit = 100;
    float pixel_to_miles = (480.0 / (max_rannge_miles * 2)); // The beginning
    int line_x = 240;
    int line_y = -920;
    double angle = 0;
    /// cause of the way we are checking mapped 29->0
    std::map<int,int> angleToLedBeam = {{171, 29}, {189, 1}, {207, 2}, {225, 3}, {243, 4}, {261, 5}, {279, 6}, {297, 7}, {315, 8}, {333, 9}, {351, 10}, {9, 11}, {27, 12}, {45, 13}, {63, 14}, {81, 15}, {99, 16}, {117, 17}, {135, 18}, {153, 19}};
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
    bool haveMapTiles = true;
    bool sdMap = false;
    QMap<quint32, Aircraft> airSpace;
    GPSReading currentGPS;
    GPSReading previousGPS;
    int pendingZoomDirection  = 0;


    /*
     * User configureable variables to set color and icon preferences.
     */
    QColor BG_COLOR = QColor(20,20,20);
    QColor SCOPE_LINES_COLOR = QColor(57,255,20);

    QColor DEFAULT_AIRPLANE = QColor(255,0,255);
    QColor AIRPLANE_ON_GROUND_COLOR = QColor(128,128,128);
    QHash<QString, QColor> AIRLINE_COLORS;

    bool RENDER_AIRPLANE_IMAGE = false;
    QString DEFAULT_AIRPLANE_IMAGE = ":/images/airplane.svg";


    void rotate(double increment);

//    void drawtraffic();

    void rotate_line_matrix(double x, double y, double angle);
    void get_planePoint(double x, double y, double angle, QColor planeColor, QString name);
    void drawPlane(QPainter *paint, QPoint coord, float distance, Aircraft *entry, bool showInfo);

    void erasePlane(int x, int y);
    void drawCirle(QPainter *paint, int val);

    void updateXY();
    QPoint rotate_matrix(double x, double y, double x_shift, double y_shift, double angle);

    bool setMapTile(double zoom, double lat, double lon);
    void changeMap();
    void changeMapLocation();

    bool updateMap(bool doForce);
    QPixmap* pixMap = nullptr;

protected:
    void paintEvent(QPaintEvent *pEvent) override;
    void hideEvent(QHideEvent *ev) override;
    void wheelEvent(QWheelEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    QTimer *zoomDebounceTimer;

private slots:

    void sweep_line();
    void getAirTraffic();
    void processDebouncedZoom();

private:
    QLabel *label;
    QMutex state_mutex;
};

} // namespace WingletUI

#endif // WINGLETUI_RADARSCOPE_H
