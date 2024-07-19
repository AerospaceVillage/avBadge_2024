#ifndef RADARSCOPE_H
#define RADARSCOPE_H
#include "getdata.h"

#include <QPainter>
#include <QWidget>
#include <QPaintEvent>
#include <QGraphicsView>
#include <QPen>
#include <QtDebug>
#include <QTimer>
#include <QHash>
#include <map>
#include <unordered_map>
#include <QList>

#define maxZoom 40
#define minZoom 1

#define SWEEPINTERVAL 50
#define DEGREES_PER_INTERVAL 10

#define AIRCRAFT_SIZE 3  // In Pixels
#define AIRCRAFT_OFFSET 1 // In Pixels, want to offset this amount to render the aircaft center. If AIRCRAFT_SIZE == 1 then offset would = 0, otherwise half rounded down

#define LOCATION_FILE "/home/defcon/test23/test2"


namespace Ui {
class radarscope;
}

class radarscope : public QWidget
{
    Q_OBJECT




public:
    explicit radarscope(QWidget *parent = nullptr);
    ~radarscope();

    void paintEvent(QPaintEvent *pEvent);
    int get_distance(float lat, float lon);
    void sweep_line();


    int ring_ranges_miles[3] = {10, 25, 50};
    int max_rannge_miles = 55;
    int age_limit = 100;
    float pixel_to_miles = (480.0 / (max_rannge_miles * 2)); // The beginning
    int line_x = 240;
    int line_y = -920;
    double angle = 0;

    int matchIndex = 0;

    /// cursor setup
    bool cursorEn = false;
    bool infoEn = false;
    int cursorX = 240;
    int cursorY = 240;

    QList<aircraft> airSpace;
    gpsCord currentGPS;

    /*
     * User configureable variables to set color and icon preferences.
     */
    QColor BG_COLOR = QColor(20,20,20);
    QColor SCOPE_LINES_COLOR = QColor(57,255,20);

    QColor DEFAULT_AIRPLANE = QColor(255,0,255);
    QColor AIRPLANE_ON_GROUND_COLOR = QColor(128,128,128);
    QHash<QString, QColor> AIRLINE_COLORS;

    bool RENDER_AIRPLANE_IMAGE = false;
    QString DEFAULT_AIRPLANE_IMAGE = ":file/assets/funny1.png";


    void rotate(double increment);

//    void drawtraffic();
    void setGPS();
    int get_bearing(float lat, float lon);

    void rotate_matrix(double x, double y, double angle);
    void get_planePoint(double x, double y, double angle, QColor planeColor, QString name);
    void drawPlane(QPainter *paint, float distance, float bearing, int aircraft_index, int infoIndex);

    void erasePlane(int x, int y);
    void wheelEvent(QWheelEvent *event);
    void drawCirle(QPainter *paint, int val);

    void getAirTraffic();
    void getPixToMiles();

    void keyPressEvent(QKeyEvent *event);
    int getClosesPlane(int x, int y);

    QPixmap* setMapTile(double zoom, double lat, double lon);
signals:
    void callData();
    void getLastPlane();
    void setPixToMiles(float ptom);
    void getGPS();

public slots:
    void scopeGPS(gpsCord localGPS);
    void setTraffic(QList<aircraft>);
private:
    Ui::radarscope *ui;
};

#endif // RADARSCOPE_H
