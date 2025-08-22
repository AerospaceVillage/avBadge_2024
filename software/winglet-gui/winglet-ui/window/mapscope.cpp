#include "mapscope.h"
#include <QPainter>
#include <QFile>
#include <cmath>
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDir>

#include "wingletgui.h"

#define earthRadiusKm 6371.0

namespace WingletUI {

MapScope::MapScope(QWidget *parent)
    : QWidget{parent}
{
    // Load the last latitude/longitude readings in
    currentGPS = GPSReading(WingletGUI::inst->settings.lastLatitude(),
                            WingletGUI::inst->settings.lastLongitude(), false);

    previousGPS = GPSReading(WingletGUI::inst->settings.lastLatitude(),
                             WingletGUI::inst->settings.lastLongitude(), false);

    setGeometry(0, 0, 480, 480);
    label = new QLabel(this);
    label->setGeometry(0, 0, 480, 480);


    // --- Initialize the Debounce Timer ---
    zoomDebounceTimer = new QTimer(this);
    zoomDebounceTimer->setSingleShot(true); // Ensures it fires only once per start()
    zoomDebounceTimer->setInterval(150);    // 150 ms debounce interval (adjust as needed)

    // Connect the timer's timeout signal to our debounced processing slot
    QObject::connect(zoomDebounceTimer, &QTimer::timeout, this, &MapScope::processDebouncedZoom);
    // Register to receive settings updates
    AppSettings *settings = &dynamic_cast<WingletGUI*>(parent)->settings;
    setSdCardMaps(settings->sdCardMaps());
//    this->resize(480, 480);
//    this->setAutoFillBackground(true);
//    QPalette pal = QPalette();
//    pal.setColor(QPalette::Window, BG_COLOR);
//    this->setPalette(pal);
//    this->show();

    QTimer* tir = new QTimer(this);
    // connect(tir, SIGNAL(timeout()), this, SLOT(sweep_line()));

    tir->start(SWEEPINTERVAL);

    //// get aircraft structy form getData()
    connect(tir, SIGNAL(timeout()), this, SLOT(getAirTraffic()));


    QFile fileP("/mnt/sd/airplane.svg");
    if(!fileP.open(QIODevice::ReadOnly)) {

    }else{
        /// if it exsicts then repalce the image with the SD card one
        DEFAULT_AIRPLANE_IMAGE = "/mnt/sd/airplane.svg";
    }
    fileP.close();
    //QHash items have to be intserted in the main method for some reason... fails when moved to the header file.
    AIRLINE_COLORS.insert(QString("SWA"), QColor(255,165,0));
    AIRLINE_COLORS.insert(QString("DAL"), QColor(0,0,255));
    AIRLINE_COLORS.insert(QString("UPS"), QColor(139,69,19));
    AIRLINE_COLORS.insert(QString("UAL"), QColor(0,0,255));
    AIRLINE_COLORS.insert(QString("AAL"), QColor(255,0,0));
    AIRLINE_COLORS.insert(QString("FDX"), QColor(162,25,255));


    getAirTraffic();
}

MapScope::~MapScope()
{
    delete label;
    WingletGUI::inst->ledControl->clearRing();
}

void MapScope::hideEvent(QHideEvent *ev) {
    (void) ev;
}

void MapScope::wheelEvent(QWheelEvent *wheelEvent) {

    if (this->viewMap == false){
        if(wheelEvent->angleDelta().y() > 0 ){
            if(this->pixel_to_miles +1.0 < maxZoom){
                this->pixel_to_miles += 1.0f;
            }
        } else if(wheelEvent->angleDelta().y() < 0){
            if(this->pixel_to_miles -1.0 > minZoom){
                this->pixel_to_miles -= 1.0f;
            }
        }
    }else {
        //setEnabled(false);  //don't respond to more wheelEvent until this one completes... map processing is expensive
        // Immediately log the raw wheelEvent for debugging

        // Determine the desired zoom direction
        if (wheelEvent->angleDelta().y() > 0) {
            // Zoom In (scroll up)
            // zoom limit is 13
            if (mapLevel < 13) { // Check if we can still zoom in
                pendingZoomDirection = 1; // Mark a zoom-in as pending
            } else {
                pendingZoomDirection = 0; // No change

            }
        } else if (wheelEvent->angleDelta().y() < 0) {
            // Zoom Out (scroll down)
            // zoom limit is 7
            if (mapLevel > 7) { // Check if we can still zoom out
                pendingZoomDirection = -1; // Mark a zoom-out as pending
            } else {
                pendingZoomDirection = 0; // No change
            }
        } else {
            // No vertical scroll
            pendingZoomDirection = 0;
        }

        // Only start the timer if there's a pending zoom action
        if (pendingZoomDirection != 0) {
            // Start or restart the debounce timer.
            // If the user scrolls again before 150ms, the timer will reset,
            // delaying the `processDebouncedZoom` call.
            zoomDebounceTimer->start();
        }

        // IMPORTANT: Call the base class implementation.
        // This is necessary if QWidget or its parents need to handle the event too.
        QWidget::wheelEvent(wheelEvent);
    }

        //state_mutex.unlock();
        //setEnabled(true);


    update();
}

void MapScope::keyPressEvent(QKeyEvent *ev) {
    if(pressedKeys.size() >= 4){
        pressedKeys.removeAt(0);
    }
    // key Combo to view map tiles as background
    pressedKeys += ev->key();
    // if(pressedKeys.at(0)== Qt::Key_Left && pressedKeys.at(1) == Qt::Key_Right && pressedKeys.at(2) == Qt::Key_Left && pressedKeys.at(3) == Qt::Key_Down ){
    //     changeMap();
    // }
    // key Combo to change map location to SD card
    if(pressedKeys.at(0)== Qt::Key_Up && pressedKeys.at(1) == Qt::Key_Up && pressedKeys.at(2) == Qt::Key_Up && pressedKeys.at(3) == Qt::Key_Right ){
        changeMapLocation();
    }
    // key Combo to enable plane images
    // if(pressedKeys.at(0)== Qt::Key_Up && pressedKeys.at(1) == Qt::Key_Down && pressedKeys.at(2) == Qt::Key_Left && pressedKeys.at(3) == Qt::Key_Right ){
    //     RENDER_AIRPLANE_IMAGE = !RENDER_AIRPLANE_IMAGE;
    // }

    switch ( ev->key() ) {
    case Qt::Key_Up:
        if(this->cursorY > 50 && this->cursorY <= 430){
            this->cursorY -= 5;
        }
        update();
        break;
    case Qt::Key_Down:
        if(this->cursorY >= 50 && this->cursorY < 430){
            this->cursorY +=5;
        }
        update();
        break;
    case Qt::Key_Right:
        if(this->cursorX >= 50 && this->cursorX < 430){
            this->cursorX += 5;
        }
        update();
        break;
    case Qt::Key_Left:
        if(this->cursorX > 50 && this->cursorX <= 430 ){
            this->cursorX -= 5;
        }
        update();
        break;

    case Qt::Key_Return:
        cursorEn = !cursorEn;
        update();
        // show cursor led
        // rgb currcolor;
        // currcolor.r = 0.3;
        // currcolor.g = 0.3;
        // currcolor.b = 0.3;
        // rgb curOffColor;
        // curOffColor.r = 0;
        // curOffColor.g = 0;
        // curOffColor.b = 0;

        // if (cursorEn == true){
        //     rgbled_set(22, currcolor);
        // }else{
        //     rgbled_set(22, curOffColor);
        // }
        // break;
        break;
    case Qt::Key_A:
        infoEn = !infoEn;
        update();
        // show info in led for planes
        // rgb incolor;
        // incolor.r = 0.3;
        // incolor.g = 0.3;
        // incolor.b = 0.0;
        // rgb offColor;
        // offColor.r = 0;
        // offColor.g = 0;
        // offColor.b = 0;

        // if (infoStatu == false){
        //     rgbled_set(23, incolor);
        // }else{
        //     rgbled_set(23, offColor);
        // }
        // infoStatu = !infoStatu;
        // break;
        break;
    case Qt::Key_S:
        changeMap();
        if (this->viewMap == false){
            this->pixel_to_miles = (480.0 / (max_rannge_miles * 2));
        } else {
            this->pixel_to_miles = mapValue[mapLevel];
        }
        update();
        break;
    case Qt::Key_I:
        RENDER_AIRPLANE_IMAGE = !RENDER_AIRPLANE_IMAGE;
        break;
    default:
        ev->ignore();
        break;
    };
}

/// whenever a update() is called this is executed
void MapScope::paintEvent(QPaintEvent *pEvent)
{
    (void) pEvent;

    // constuct painter
    QPixmap *pix = new QPixmap(480,480); /// in reality is 480,480
    QPainter *painter = new QPainter(pix);
    painter->fillRect(rect(), activeTheme->palette.color(QPalette::Window));


    // if (viewMap == true){
        updateMap(false);
        // qDebug() << "haveMapTiles: "<< haveMapTiles;
        if(haveMapTiles == true){
            painter->drawPixmap(0,0,*pixMap);
        }else {
                painter->drawImage(115,115,QImage(":/images/NOMAP.jpg"));
        }
        // Show what zoom level you are
        QFont bold(activeTheme->standardFont, 12); // or Fantasy or AnyStyle or Helvetica
        painter->setFont(bold);
        painter->setPen(Qt::darkRed);
        QRect textRect(200, 450, 200, 200);
        painter->drawText(textRect,1, QStringLiteral("Zoom Level: %1").arg(this->mapLevel));
        if(sdMap == true){
            if (sdMapDir == false){
                painter->drawImage(60,60,QImage(":/images/NOSDMAP.jpg"));
                haveMapTiles = false;
            }
            QRect sdRect(150,420,100,100);
            painter->drawText(sdRect,1, QStringLiteral("SD Maps"));
        }

    // }

    // QFont bold(activeTheme->standardFont, 12); // or Fantasy or AnyStyle or Helvetica
    // painter->setFont(bold);

    // if (viewMap == false){

    //     //Draw the sweeping line, the rings and the text labels
    //     //QPen pen(SCOPE_LINES_COLOR);
    //     QPen pen(*activeTheme->radar_sweep_lines_color);
    //     pen.setWidth(2);
    //     painter->setPen(pen);
    //     QLine rotLine(240,240,this->line_x,this->line_y);
    //     painter->drawLine(rotLine);

    //     // set up the rings for area
    //     for(int x : this->ring_ranges_miles) {
    //         drawCirle(painter,x);
    //     }
    //     if(this->pixel_to_miles >= 15){
    //         drawCirle(painter,5);
    //     }
    //     if(this->pixel_to_miles <= 5){
    //         drawCirle(painter,100);
    //     }
    //     if(this->pixel_to_miles <= 3){
    //         drawCirle(painter,150);
    //     }
    // }

    // draw Plane in the painting x, y, color
    bool infoAvailable = (infoEn && cursorEn);
    for (auto& entry : airSpace) {
        if (entry.latValid & entry.lonValid){
            // Compute positioning on screen
            //float distance = distanceEarth(entry.lat, entry.lon);
            //float bearing = get_bearing(entry.lat, entry.lon);
            int start_y = int(240 - entry.distance * this->pixel_to_miles);     //Distance calculation refactored to adsbreciever
            QPoint coord = rotate_matrix(240, start_y, 0, 0, entry.bearing);    //Bearing calculation refactored to adsbreciever

            // Check if extra info needs to be shown
            bool showInfo = false;
            if (infoAvailable && abs(cursorX - coord.x()) < 10 && abs(cursorY - coord.y()) < 10) {
                showInfo = true;
                infoAvailable = false;
            }

            // Draw the plane
            drawPlane(painter, coord, entry.distance, &entry, showInfo);
          }
    }

    //draw cursor
    if(cursorEn == true){
        QColor cursorColor = *activeTheme->radar_sweep_cursor_color_inv;
        QPen cursorPen(cursorColor);
        cursorPen.setWidth(2);
        painter->setPen(cursorPen);
        QColor cursorCenter(0,133,0);
        cursorCenter.setAlphaF( 0.0 );
        painter->setBrush(cursorCenter);
        painter->setBrush(cursorCenter);
        painter->drawEllipse(this->cursorX-10,this->cursorY-10,20,20);
        painter->drawLine(this->cursorX,this->cursorY-10,this->cursorX,this->cursorY-20);
        painter->drawLine(this->cursorX,this->cursorY+10,this->cursorX,this->cursorY+20);
        painter->drawLine(this->cursorX-20,this->cursorY,this->cursorX-10,this->cursorY);
        painter->drawLine(this->cursorX+10,this->cursorY,this->cursorX+20,this->cursorY);
    }

    //If information is attempting to be displayed render an "i" at the topif (infoEn == true /*&& cursorEn == true*/){
    if (infoEn == true /*&& cursorEn == true*/){
        QFont bold1(activeTheme->titleFont, 18);
        bold1.setBold(true); // or Fantasy or AnyStyle or Helvetica
        painter->setFont(bold1);
        painter->setPen(QPen(Qt::yellow));
        painter->drawText(235, 30,QString("I"));
    }

    label->setPixmap(*pix);     //To Do - later - label is an odd variable name for that actual radar scope things to be painted to
    delete painter;
    delete pix;
}

QPoint MapScope::rotate_matrix(double x, double y, double x_shift, double y_shift, double angle){
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


    QPoint tmp;

    tmp.setX((x * c) - (y * s) + x_shift);
    tmp.setY((x * s) + (y * c) + y_shift);

    return tmp;
}


/// draw circles for the range form the center
void MapScope::drawCirle(QPainter *paint, int val){
    float ring = 2.0*val*this->pixel_to_miles;
    int center = ((480-ring)/2);
    //QPen pen(SCOPE_LINES_COLOR);
    //paint->setPen(pen);
    paint->drawArc(center, center, ring, ring, 0, 16*360);
    //Draw the ring labels
    paint->drawText(231,(this->pixel_to_miles*(val)+250), QString::number(val));
}


/// rotate the line to look like scanning a radar
void MapScope::rotate(double increment ){
    this->angle += increment;
    // after 360 degress it rotates around different values
    if (this->angle >= 360){
        this->line_x = 240;
        this->line_y = -920;
        this->angle = 0;
    }
    rotate_line_matrix(240,240, increment); /// rotate around 240,240
    // qDebug() << this->angle;
    if(angleToLedBeam[this->angle]){
        /// becauce of angleToLedBeam[this->angle] return 0 the statment foes to else so mapping 29 to 0
        if (angleToLedBeam[this->angle] == 29)
            WingletGUI::inst->ledControl->setRingLed(0,0,255,0);
        else
            WingletGUI::inst->ledControl->setRingLed(angleToLedBeam[this->angle],0,255,0);
    } else {
        WingletGUI::inst->ledControl->clearRing();
    }

    //rgbled_set_angle(this->angle, on_color, off_color);
    // rgbled_show_radar_beam(this->angle);

}


/// do the math for scanning points location
void MapScope::rotate_line_matrix(double x, double y, double angle ){
    // shift to origin (0,0)
    angle = angle * (M_PI/180);
    double s = sin(angle);
    double c = cos(angle);
    this->line_x -= x;
    this->line_y -= y;

    double xr = (this->line_x * c) - (this->line_y * s) ;
    double yr = (this->line_x * s) + (this->line_y * c) ;
    this->line_x = xr + x;
    this->line_y = yr + y;

}


/// draw plane in given location x,y and color
void MapScope::drawPlane(QPainter *paint, QPoint coord, float distance, Aircraft *entry, bool showInfo){
    if (entry->latValid && entry->lonValid){
        if(RENDER_AIRPLANE_IMAGE == true && entry->planeTrackValid){
            QImage test = QImage(DEFAULT_AIRPLANE_IMAGE);
            test = test.scaled(20, 20, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            QTransform transform = QTransform();
            transform.rotate(entry->planeTrack);
            test = test.transformed(transform);
            paint->drawImage(QPoint(coord.x()-8,coord.y()-8), test);
        }else{

            QColor color = DEFAULT_AIRPLANE;

            QList<QString> keys = AIRLINE_COLORS.keys();
            if (entry->callSignValid) {
                foreach( auto key, keys){
                    if (entry->callSign.indexOf(key) == 0){
                        QHash<QString, QColor>::const_iterator i = AIRLINE_COLORS.find(key);
                        color = i.value();
                        break;
                    }
                }
            }

            if (entry->onGroundValid && entry->isOnGround == true){
                color = AIRPLANE_ON_GROUND_COLOR;
            }
            paint->setBrush(QBrush(color));
            paint->setPen(QPen(color));

            QRect r1(QPoint(coord.x()-AIRCRAFT_OFFSET, coord.y()-AIRCRAFT_OFFSET),QSize(AIRCRAFT_SIZE, AIRCRAFT_SIZE));
            paint->drawEllipse(r1);
        }

        //If the cursor is hoovering over this plane then render the details of it.
        if(showInfo){
            QFont bold(activeTheme->standardFont, 10); // or Fantasy or AnyStyle or Helvetica
            paint->setFont(bold);
            if(entry->callSignValid){
                paint->drawText(coord.x()-40, coord.y()-30,QString(entry->callSign));
            } else {
                paint->drawText(coord.x()-40, coord.y()-30, QString::number(entry->icao24, 16).rightJustified(6, '0').toUpper());
            }
            QString distanceStr = QString::number(distance, 'f', 2) + "nm";
            paint->drawText(coord.x()-40, coord.y()-20, distanceStr);
        }
    }
}


/// set the sweeping speed
void MapScope::sweep_line(){
    // rotate the defined interval step amount
    rotate(DEGREES_PER_INTERVAL);

}

//// calls get data to return aircraft stuct
void MapScope::getAirTraffic(){
    airSpace = WingletGUI::inst->adsbReceiver->airspace();
    auto reading = WingletGUI::inst->gpsReceiver->lastReading();
    if (reading.valid) {
        currentGPS = reading;
    }
    update();
}

void MapScope::changeMap(){
   viewMap = !viewMap;
}


void MapScope::changeMapLocation()
{
    sdMap = !sdMap;
}
/// if true then force an update by other means then the paint event
/// paint event set to false to check if GPS changed or image NULL
bool MapScope::updateMap(bool doForce){
    float diff_lat = ((currentGPS.latitude * 100.0)/100) - ((previousGPS.latitude * 100.0)/100);
    float diff_lon = ((currentGPS.longitude * 100.0)/100) - ((previousGPS.longitude * 100.0)/100);

    bool status;

    if(doForce == true){
        status = setMapTile(this->mapLevel, currentGPS.latitude, currentGPS.longitude);
        if(status == true){
            haveMapTiles = true;
        }else{
            haveMapTiles = false;
        }
        previousGPS = currentGPS;
        return status;
    }
    if((diff_lat > 0) | (diff_lon > 0)){
        status = setMapTile(this->mapLevel, currentGPS.latitude, currentGPS.longitude);
        if(status == true){
            haveMapTiles = true;
        }else{
            haveMapTiles = false;
        }
        previousGPS = currentGPS;

        return status;
    }else if(pixMap == nullptr){
        status = setMapTile(this->mapLevel, currentGPS.latitude, currentGPS.longitude);
        if(status == true){
            haveMapTiles = true;
        }else{
            haveMapTiles = false;
        }
        return status;
    }

    return false;
}

bool MapScope::setMapTile(double zoom, double lat, double lon){

    state_mutex.lock();

    double n = pow(2,zoom);
    double lat_rad = lat*M_PI/180.0;;
    double x = (lon + 180.0) / 360.0 * n;
    double y = (1.0 - asinh(tan(lat_rad)) / M_PI) / 2.0 * n;

    int zo = zoom;
    double x_shift = 0;
    double y_shift = 0;

    double pixOfImage = 256; // 256 or 512
//    double pixOfImage = 512; // 256 or 512

    double correction;
    if(pixOfImage == 256){
        correction = 16;
    }
    if(pixOfImage == 512){
        correction = 272;
    }


//   double stile = 40075016.686 * (cos(lat)/pow(2,zo));
//   double spix = stile/256;

    x_shift = modf(x,&x);
    y_shift = modf(y,&y);
    x_shift = pixOfImage - x_shift*pixOfImage - correction; /// TODO: fix the mapping accuacy
    y_shift = (-1*y_shift*pixOfImage)- correction;

    if (sdMap == true){
        QDir mapsDir;
        QString sdMapFile = SD_MAP_FILES;
        mapsDir.setPath(sdMapFile);
        if (!mapsDir.exists()) {
            sdMapDir = false;
            state_mutex.unlock();
            return false;
        }else{
            sdMapDir = true;
        }
        mapLocation = new QString(SD_MAP_FILES);
    }else{
        mapLocation = new QString(LOCAL_MAP_FILES);
    }

    //  qDebug() << "yshift: " << y_shift <<" x_shift: "<< x_shift << "correction: " << correction;
    //  String fileName = QStringLiteral("/home/defcon/Downloads/mbutil/test2/%1/%2/%3.pbf").arg().arg().arg();

    //  qDebug() << QString(LOCATION_FILE)+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y);
    // qDebug() << "path Locaiton" << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    QString zoomLength = QString::number(zo); // Set the appropriate zoom level

    QString *fileTopLeft = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y-1)); // x-1 y-1
    QImage *rawTopLeft = new QImage(*fileTopLeft,zoomLength.toLatin1());
    delete fileTopLeft;
    if(rawTopLeft->isNull()){
        delete rawTopLeft;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawTopLeft" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y-1);
        return false;
    }

    QString *fileTopCen = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y-1)); // y-1
    QImage *rawTopCen = new QImage(*fileTopCen,zoomLength.toLatin1());
    delete fileTopCen;
    if(rawTopCen->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawTopCen" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y-1);
        return false;
    }

    QString *fileTopRight = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y-1)); // x+1 y-1
    QImage *rawTopRight = new QImage(*fileTopRight,zoomLength.toLatin1());
    delete fileTopRight;
    if(rawTopRight->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        delete rawTopRight;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawTopRight" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y-1);
        return false;
    }

    QString *fileCenLeft = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y)); // x-1
    QImage *rawCenLeft = new QImage(*fileCenLeft,zoomLength.toLatin1());
    delete fileCenLeft;
    if(rawCenLeft->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        delete rawTopRight;
        delete rawCenLeft;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawCenLeft" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y);
        return false;

    }

    QString *fileCenCen = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y)); // Replace with your PBF file path
    QImage *rawCenCen = new QImage(*fileCenCen,zoomLength.toLatin1());
    delete fileCenCen;
    if(rawCenCen->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        delete rawTopRight;
        delete rawCenLeft;
        delete rawCenCen;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawCenCen" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y);
        return false;

    }

    QString *fileCenRight = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y)); // x+1
    QImage *rawCenRight = new QImage(*fileCenRight,zoomLength.toLatin1());
    delete fileCenRight;
    if(rawCenRight->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        delete rawTopRight;
        delete rawCenLeft;
        delete rawCenCen;
        delete rawCenRight;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawCenRight" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y);
        return false;

    }

    QString *fileBotLeft = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y+1)); // x-1 y+1
    QImage *rawBotLeft = new QImage(*fileBotLeft,zoomLength.toLatin1());
    delete fileBotLeft;
    if(rawBotLeft->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        delete rawTopRight;
        delete rawCenLeft;
        delete rawCenCen;
        delete rawCenRight;
        delete rawBotLeft;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawBotLeft" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x-1).arg(y+1);
        return false;
    }

    QString *fileBotCen = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y+1)); // y+1
    QImage *rawBotCen = new QImage(*fileBotCen,zoomLength.toLatin1());
    delete fileBotCen;
    if(rawBotCen->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        delete rawTopRight;
        delete rawCenLeft;
        delete rawCenCen;
        delete rawCenRight;
        delete rawBotLeft;
        delete rawBotCen;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawBotCen"<< QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x).arg(y+1);
        return false;

    }

    QString *fileBotRight = new QString(mapLocation+QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y+1)); // x+1 y+1
    QImage *rawBotRight = new QImage(*fileBotRight,zoomLength.toLatin1());
    delete fileBotRight;
    if(rawBotRight->isNull()){
        delete rawTopCen;
        delete rawTopLeft;
        delete rawTopRight;
        delete rawCenLeft;
        delete rawCenCen;
        delete rawCenRight;
        delete rawBotLeft;
        delete rawBotCen;
        delete rawBotRight;
        state_mutex.unlock();
        // qDebug() << "setMapTiles error in rawBotRight" << QStringLiteral("/%1/%2/%3.pbf").arg(zo).arg(x+1).arg(y+1);
        return false;
    }

    int wid = pixOfImage;
    int hgt = wid;
    // Display the image in a label
    QRectF *topLeft = new QRectF((wid*-1)+x_shift,0+y_shift,wid,hgt);
    QRectF *topCenter = new QRectF(0+x_shift,0+y_shift,wid,hgt);
    QRectF *topRight = new QRectF(wid+x_shift,0+y_shift,wid,hgt);
    QRectF *centerLeft = new QRectF((wid*-1)+x_shift,hgt+y_shift,wid,hgt);
    QRectF *centerCenter = new QRectF(0+x_shift,hgt+y_shift,wid,hgt);
    QRectF *centerRight = new QRectF(wid+x_shift,hgt+y_shift,wid,hgt);
    QRectF *bottomLeft = new QRectF((wid*-1)+x_shift,hgt*2+y_shift,wid,hgt);
    QRectF *bottomCenter = new QRectF(0+x_shift,hgt*2+y_shift,wid,hgt);
    QRectF *bottomRight = new QRectF(wid+x_shift,hgt*2+y_shift,wid,hgt);

    pixMap = new QPixmap(wid*4,hgt*4); /// in reality is 480,480
    QPainter *p = new QPainter(pixMap);

    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*topLeft,*rawTopLeft);
    p->end();
    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*topCenter,*rawTopCen);
    p->end();
    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*topRight,*rawTopRight);
    p->end();

    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*centerLeft,*rawCenLeft);
    p->end();
    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*centerCenter,*rawCenCen);
    p->end();
    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*centerRight,*rawCenRight);
    p->end();

    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*bottomLeft,*rawBotLeft);
    p->end();
    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*bottomCenter,*rawBotCen);
    p->end();
    p->begin(pixMap);
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->drawImage(*bottomRight,*rawBotRight);
    p->end();


    delete topLeft;
    delete topCenter;
    delete topRight;
    delete centerLeft;
    delete centerCenter;
    delete centerRight;
    delete bottomLeft;
    delete bottomCenter;
    delete bottomRight;

    delete rawTopCen;
    delete rawTopLeft;
    delete rawTopRight;
    delete rawCenLeft;
    delete rawCenCen;
    delete rawCenRight;
    delete rawBotLeft;
    delete rawBotCen;
    delete rawBotRight;

    delete p;

    state_mutex.unlock();

    return true;
}
void MapScope::processDebouncedZoom()
{
    // qDebug() << "--- Debounced zoom processing triggered! ---";

    // Reset the pending zoom direction immediately, as we are now processing it
    int currentZoomDirection = pendingZoomDirection;
    pendingZoomDirection = 0;

    if (currentZoomDirection == 1) { // Zoom In
        this->mapLevel += 1;

        bool success = updateMap(true);
        // qDebug() << "status updatemap" << success;
        if (success == true) {
            // qDebug() << "Actually ZOOMING IN. New mapLevel:" << this->mapLevel;
            this->pixel_to_miles = mapValue[mapLevel];
        } else {
            // qDebug() << "Didn't find a map to load. Undoing zoom IN.";
            this->pixel_to_miles = mapValue[mapLevel];
        }
    } else if (currentZoomDirection == -1) { // Zoom Out
        this->mapLevel -= 1;
        bool success = updateMap(true);
        // qDebug() << "status updatemap" << success;

        if (success == true) {
            // qDebug() << "Actually ZOOMING OUT. New mapLevel:" << mapLevel;
            this->pixel_to_miles = mapValue[mapLevel];
        } else {
            // qDebug() << "Didn't find a map to load. Undoing zoom OUT.";
            this->pixel_to_miles = mapValue[mapLevel];
        }
    }

    // qDebug() << "Zoom processing complete. Current mapLevel:" << mapLevel << "pixel_to_miles:" << pixel_to_miles;
    // qDebug() << "--- EEEEEEEEEEEEEEEEEnnnnnnnnnnDDDDDDDDD ---";
}

void MapScope::setSdCardMaps(bool val){
    sdMap = val;
}

} // namespace WingletUI
