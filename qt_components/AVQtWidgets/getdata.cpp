/*****
All Message
Location Tag
    0    MSG
    1	 MessageType
    4    adsHex
    8    dateStamp
    9    timeStamp

    Depends on messages
    MessageType 1
    10    Callsign

    MessageType 3
    11    altitude
    14    latitude
    15    longitude

    MessageType 6
    17    squawk

    MessageType 4
    12    gnd_speed

*****/

#include "getdata.h"
//#include "ui_getdata.h"
#include <QDebug>
#include <cmath>
#include <QDateTime>




//struct aircraft myship;
getData::getData(QWidget *parent) :
    QWidget(parent)/*,*/
//    ui()
//    ui(new Ui::getData)
{
    // <-- fill with data

}

getData::~getData()
{
    delete this;
}


/// dedicated function for start
void getData::start(){
    _pSocket = new QTcpSocket( this ); // <-- needs to be a member variable: QTcpSocket * _pSocket;
    _pSocket->connectToHost(dump1090_IP, dump1090_PORT);
    connect( _pSocket, SIGNAL(readyRead()), this, SLOT(readSocketData()) );
    qDebug() << "connecting...";
//    qDebug() << "Size of aicraft: " << sizeof(aircraft);

}

//// Example Messge layout i am reading here
////
//// MSG,4,111,11111,A24014,111111,2024/06/26,20:27:45.998,2024/06/26,20:27:45.961,,,231,0,,,-576,,,,,0
//// MSG,7,111,11111,A35220,111111,2024/06/26,20:27:46.061,2024/06/26,20:27:46.026,,10175,,,,,,,,,,0
//// MSG,7,111,11111,AB9B13,111111,2024/06/26,20:27:46.102,2024/06/26,20:27:46.090,,11300,,,,,,,,,,0
//// MSG,7,111,11111,A24014,111111,2024/06/26,20:27:46.239,2024/06/26,20:27:46.221,,11800,,,,,,,,,,0
//// MSG,8,111,11111,A35220,111111,2024/06/26,20:27:46.245,2024/06/26,20:27:46.222,,,,,,,,,,,,0
//// MSG,7,111,11111,AD24AB,111111,2024/06/26,20:27:46.401,2024/06/26,20:27:46.357,,12975,,,,,,,,,,0
//// MSG,7,111,11111,AD24AB,111111,2024/06/26,20:27:46.486,2024/06/26,20:27:46.483,,12975,,,,,,,,,,0
//// MSG,7,111,11111,AB9B13,111111,2024/06/26,20:27:46.537,2024/06/26,20:27:46.488,,11325,,,,,,,,,,0
//// MSG,5,111,11111,AD24AB,111111,2024/06/26,20:27:46.586,2024/06/26,20:27:46.553,,12975,,,,,,,0,,0,0
//// MSG,7,111,11111,AD24AB,111111,2024/06/26,20:27:46.593,2024/06/26,20:27:46.554,,12975,,,,,,,,,,0
//// MSG,7,111,11111,AAE73B,111111,2024/06/26,20:27:46.619,2024/06/26,20:27:46.614,,7450,,,,,,,,,,0
///
///


/// get data form socket
/// will be different on badge
void getData::readSocketData(){

    setGPS();

    //So as to not slow down the socket... read A LOT and then parse for rows
    int BUF_SIZE = 4096;
    QString dataStr = QString(_pSocket->read(BUF_SIZE));

    int start = dataStr.indexOf("\r\n") + 2;
    int end = dataStr.lastIndexOf("\r\n") + 2;

    dataStr = dataStr.mid(start, end);

    QStringList rows = dataStr.split("\r\n");
    for(int i=0; i<rows.count()-1; i++){
        QStringList query = rows[i].split(",");
        if (query.count() > 1){
            int ind = 0;
            ind = getEmptyIndex(query[4]);
            /// based on BaseStation format info will be in different locations
            if(query[0] == "MSG"){
                *this->myAIR[ind].hexAddr = query[4];
                *this->myAIR[ind].messageType = query[1];
                *this->myAIR[ind].dateStamp = query[8];
                *this->myAIR[ind].timeStamp = query[9];
                if(query[1] == '1'){
                   *this->myAIR[ind].callSign = query[10];
                }
                if(query[1] == "2" || query[1] == "3" || query[1] == "5" || query[1] == "6" || query[1] == "7"){
                   this->myAIR[ind].alt = query[11].toInt();
                }
                if(query[1] == "2" || query[1] == "4"){
                    this->myAIR[ind].gndSpeed = query[12].toInt();
                    this->myAIR[ind].planeTrack = query[13].toFloat();
                }
                if(query[1] == "2" || query[1] == "3"){
                    this->myAIR[ind].lat = query[14].toFloat();
                    this->myAIR[ind].lon = query[15].toFloat();

                    this->myAIR[ind].distance = distanceEarth(this->myAIR[ind].lat,this->myAIR[ind].lon);
                    this->myAIR[ind].bearing = get_bearing(this->myAIR[ind].lat,this->myAIR[ind].lon);

                    //Update the x_coord and y_coord points as they arrive.
                    int start_y = int(240 - myAIR[ind].distance * this->pixel_to_miles);
                    xyPAIR tmp = rotate_matrix(240, start_y, 0, 0, myAIR[ind].bearing);

                    myAIR[ind].x_coord = int(tmp.x);
                    myAIR[ind].y_coord = int(tmp.y);
                }
                if(query[1] == "6"){
                    this->myAIR[ind].squawk = query[17].toInt();
                }
                if(query[1] == "2" || query[1] == "3" || query[1] == "5" || query[1] == "6" || query[1] == "7" || query[1] == "8"){
                    if(query[21] == "1"){
                        this->myAIR[ind].isOnGround = true;
                    }
                    if(query[21] == "0"){
                        this->myAIR[ind].isOnGround = false;
                    }
                }

            }
            deletePlaneTime();
            //qDebug() << "Traffic Count: " << myAIR.size() << "Current ind:" << ind;
        }
    }
}

/// get the index of the plane based on hexAddr
int getData::getEmptyIndex(QString hexNum){
//    int a = AIRCRAFTLIMIT;
////    QString  NONAME = new QString("NONAME");
//    for (int i = 0; i < a-1; i++){
//        int o = this->myAIR[i].hexAddr->compare("NONAME");
//            if(o == 0){
////                qDebug() << "empty i returned " << i;
//                return i;
//            }
//            if(this->myAIR[i].hexAddr->compare(&hexNum) == 0){
////                qDebug() << "match i returned " << i;
//                return i;
//            }

//    }
    for(int i = 0; i < myAIR.size(); i++){
        if(this->myAIR[i].hexAddr->compare(&hexNum) == 0){
            return i;
        }
    }
    int tmp = myAIR.size();
    struct aircraft emptyStruct;
    myAIR.append(emptyStruct);
//    delete emptyStruct;
    return tmp;
}

/// when QWidget calls givePlane signal to return aircraft struct
void getData::onReqAirSpace(){
//    qDebug() << "in getData "<< ind;
    emit givePlane(this->myAIR);

}

/// return the last plane in the array + 1 since it checks empty slots
//void getData::lastPlane()
//{
////    int a= AIRCRAFTLIMIT;
////    for (int i = 0; i < a-1; i++){
////        int o = this->myAIR[i].hexAddr->compare("NONAME");
////            if(o == 0){
//////                qDebug() << "empty i returned " << i;
////                this->trafficLength = i;
////                giveLastPlane(i);
////                return;
////            }
////    }
//    giveLastPlane(myAIR.size());
//}

/// sets the pixel_to_miles form radarscope
void getData::setPtoM(float pixToMiles){
//    qDebug() << "Updating Pixels to Miles new value: " << pixToMiles;
    this->pixel_to_miles = pixToMiles;
    updateXY();
}

/// sets GPS for the board
void getData::setGPS(){

    // what ever we need to set current gps
    this->currentGPS.lat = GPS_LAT;
    this->currentGPS.lon = GPS_LON;
}

/// Functions call to get current GPS sends GPS stuct
/// by updateGPS signal
void getData::giveGPS()
{
    emit updateGPS(this->currentGPS);
}


/**
 * Returns the distance between two points on the Earth.
 * Direct translation from http://en.wikipedia.org/wiki/Haversine_formula
 * @param lat1d Latitude of the first point in degrees
 * @param lon1d Longitude of the first point in degrees
 * @param lat2d Latitude of the second point in degrees
 * @param lon2d Longitude of the second point in degrees
 * @return The distance between the two points in nautical miles
 */
double getData::distanceEarth(double lat2d, double lon2d) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = currentGPS.lat * M_PI / 180;
  lon1r = currentGPS.lon * M_PI / 180;
  lat2r = lat2d * M_PI / 180;
  lon2r = lon2d * M_PI / 180;
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v)) * 0.539957;
}

/// calculates plane bearing
float getData::get_bearing(float lat, float lon){
    /// center location scope
    float startLat = currentGPS.lat * (M_PI/180);
    float startLon = currentGPS.lon * (M_PI/180);

    /// location of plane
    float endLat = lat * (M_PI/180);
    float endLon = lon * (M_PI/180);

    float dLong = endLon - startLon;

    float dPhi = log(tan(endLat/2.0 + M_PI/4.0)/tan(startLat/2.0 + M_PI/4.0));
    if(abs(dLong) > M_PI){
        if (dLong > 0.0){
            dLong = -(2.0 * M_PI - dLong);
        } else {
            dLong = (2.0 * M_PI + dLong);
        }
    }
    float bearing = fmod(((atan2(dLong, dPhi)*(180/M_PI)) + 360.0), 360.0);

//    qDebug() << " bearing num " << bearing;
    return bearing;
}


/// updates the XY of the plane based on the change of pixel_to_miles
void getData::updateXY(){
    for (int i = 0; i < myAIR.size(); i++){
        if (myAIR[i].hexAddr != QString("NONAME")){
            int start_y = int(240 - myAIR[i].distance * pixel_to_miles);
            xyPAIR tmp = rotate_matrix(240, start_y, 0, 0, myAIR[i].bearing);

            myAIR[i].x_coord = int(tmp.x);
            myAIR[i].y_coord = int(tmp.y);
        }
    }
//  qDebug() << "updateXY completing, trafficLength" << trafficLength;
}

/*
 * Periodically planes need to be pruned from myAIR as they age out.
 */
void getData::deletePlaneTime(){
    QDateTime currentTime = QDateTime::currentDateTimeUtc();
//    struct aircraft emptyStruct;
//    if(myAIR[0].hexAddr != QString("NONAME")){
        for(int i = 0; i < myAIR.size(); i++){
            QString strTime = QString(*this->myAIR[i].dateStamp + " " +this->myAIR[i].timeStamp->left(8));
            //// find the message time and compare
            QDateTime last_msg  = QDateTime::fromString(strTime,"yyyy/MM/dd HH:mm:ss");
//            last_msg.setTimeSpec(Qt::UTC);
            qint64 diff = currentTime.secsTo(last_msg);
            if(abs(diff) > AGE_THRESHOLD){
                qDebug() << "Purging index [" << i << "] for age diff: " << diff << currentTime << " airplane time: " << last_msg;
                myAIR.removeAt(i);
            }
//        qDebug() << i <<" arr";
//        qDebug() << planeTime << " form array " << currentTime;
//        }
    }

}


xyPAIR getData::rotate_matrix(double x, double y, double x_shift, double y_shift, double angle){
    if(x_shift == 0){
        x_shift = 240;
    }
    if(y_shift == 0){
        y_shift = 240;
    }

    x = x - x_shift;
    y = y - y_shift;

    angle = angle * (M_PI/180);
    double s = sin(angle);
    double c = cos(angle);


    xyPAIR tmp = xyPAIR();

    tmp.x = (x * c) - (y * s) + x_shift;
    tmp.y = (x * s) + (y * c) + y_shift;

    return tmp;
}
