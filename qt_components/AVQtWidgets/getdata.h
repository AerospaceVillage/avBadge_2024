#ifndef GETDATA_H
#define GETDATA_H

#include <QWidget>
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qudpsocket.h>
#include <QTimeZone>
#include <bits/stl_vector.h>
#include <string>
#include <vector>
#include <cmath>
#include <QByteArray>
#include <algorithm>
#include <math.h>
#include <QPointer>

#define GPS_LAT 32.845469
#define GPS_LON -97.258741
// #define GPS_LAT 38.85665
// #define GPS_LON -77.61506
//#define SECONDS_OF_LAST_PLANE_MESS 90 // default 90
#define dump1090_IP "192.168.1.233"
// #define dump1090_IP "192.168.160.232"
#define dump1090_PORT 30003
#define MAX_NUM_PLANES 50

#define AIRCRAFTLIMIT 100
#define PIXELTOMILES_START 4

#define AGE_THRESHOLD 10    // To be interpretted in SECONDS
#define earthRadiusKm 6371.0


//namespace Ui {
//class getData;
//}

/* Structure used to describe an aircraft in iteractive mode. */
struct aircraft {
    QString* messageType = new QString("000000");   /* Message recieved type */
    QString* hexAddr = new QString("NONAME");    /* ADSHex ** Key ID for building array ** */
    QString* callSign = new QString("00000");     /* Flight number */
    int alt = 0;       /* Altitude */
    float lat = 0 ;         /* Latitude */
    float lon = 0;         /* Longitude */
    int speed = 0 ;          /* Velocity computed from EW and NS components. */
    int track = 0;          /* Angle of flight. */
    QString* dateStamp = new QString("0000/00/00");     /* date stamp the message was recieved */
    QString* timeStamp = new QString("00:00:00.000");     /* time stamp the message was recieved */
    int squawk = 0;         /* squawk */
    int gndSpeed = 0;       /* Ground Speed */
    float distance = -1;
    float bearing = -1;
    int x_coord = 0;
    int y_coord = 0;
    float planeTrack = 0;
    bool isOnGround = false;
//    struct aircraft *next; /* next aircraft in our linked list */
} ;

struct gpsCord {
    float lon = GPS_LON;
    float lat = GPS_LAT;
};


struct xyPAIR {
    float x;
    float y;
};


class getData : public QWidget
{
    Q_OBJECT

public:
    explicit getData(QWidget *parent = nullptr);
    ~getData();
    QByteArray getADSBData();
    QTcpSocket *_pSocket;
    //QUdpSocket *_pSocket;


    float pixel_to_miles = 480/110;   // TODO: figure out what the first setting should be based on the initial radarScope settings
    int active_plane_count = 0;
    void start();
//    aircraft myAIR[AIRCRAFTLIMIT+1]; /// the 51 is a end of array indicator
    QList<aircraft> myAIR;
    int getEmptyIndex(QString hexNum);
    int trafficLength = 0;
    gpsCord currentGPS;

    void get_planePoint(double x, double y, double angle, int index);
    float get_bearing(float lat, float lon);
    double distanceEarth(double lat2d, double lon2d);
    void updateXY();
    xyPAIR rotate_matrix(double x, double y, double x_shift, double y_shift, double angle);

    void setGPS();
    void deletePlaneTime();

public slots:
    void readSocketData();
    void setPtoM(float pixToMiles);
    void giveGPS();
    void onReqAirSpace();

signals:
    void givePlane(QList<aircraft>);
    void updateGPS(gpsCord);

private:
//    Ui::getData *ui;
};



#endif // GETDATA_H
