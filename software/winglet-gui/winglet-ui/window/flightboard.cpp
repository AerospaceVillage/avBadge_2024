#include "flightboard.h"
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include <QPainter>

namespace WingletUI {

#define GPS_EXPIRE_TIME_MS 30000

FlightBoard::FlightBoard(QWidget *parent)
    : QWidget{parent}
{
    // Load the last latitude/longitude readings in
    currentGPS = GPSReading(WingletGUI::inst->settings.lastLatitude(),
                            WingletGUI::inst->settings.lastLongitude(), false);

    setGeometry(0, 0, 480, 480);
    label = new QLabel(this);
    label->setGeometry(QRect(0, 0, 480, 480));

    QTimer* tir = new QTimer(this);
    getAirTraffic();

    avLogoLabel = new QLabel(this);
    activeTheme->renderBgAvLogo(avLogoLabel);

    //// get aircraft structy form getData()
    connect(tir, SIGNAL(timeout()), this, SLOT(getAirTraffic()));
    // start is base on milliseconds
    tir->start(100);
}

FlightBoard::~FlightBoard()
{
    delete avLogoLabel;
    delete label;
}

/// whenever a update() is called this is executed
void FlightBoard::paintEvent(QPaintEvent *pEvent)
{
    (void) pEvent;

    /// constuct painter
    QPixmap pix(480, 480); /// in reality is 480,480
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setBackgroundMode(Qt::TransparentMode);

    QColor fontL = palette().color(QPalette::Text);
    QRect gpsLock(160,30,40,20);

    painter.setPen(fontL);
    painter.setFont(QFont(activeTheme->standardFont, 12, QFont::Bold));

    if (currentGPS.valid && currentGPS.timestamp.isValid() &&
            abs(currentGPS.timestamp.msecsTo(QDateTime::currentDateTimeUtc())) < GPS_EXPIRE_TIME_MS) {
        painter.drawText(gpsLock, "Lock");
    }
    else {
        painter.drawText(gpsLock, "STALE");
    }

    /// Set up Font and Color
    QColor fontC = palette().color(QPalette::WindowText);
    QFont bold(activeTheme->standardFont, 10, QFont::Bold); // or Fantasy or AnyStyle or Helvetica
    painter.setPen(fontC);
    painter.setFont(bold);

    /// Setup display for GPS
    QRect gps(180,20,40,20);
    QRect gpsLat(220,10,150,20);
    QRect gpsLon(220,25,150,20);
    painter.drawText(gps,0, "GPS:");
    painter.drawText(gpsLat, 0, QString::number(currentGPS.latitude));
    painter.drawText(gpsLon, 0, QString::number(currentGPS.longitude));

    /// Draw table
    /// Draw the header of the table
    int spacingX = 50;
    int spacingY = 20;
    int startY = 70;
    int startX = 90;
    painter.drawText(startX-5, startY, "ICAO24");
    painter.drawText(startX+spacingX, startY, "FLIGHT");
    painter.drawText(startX+(spacingX*2+20), startY, "ALT");
    painter.drawText(startX+(spacingX*3+20), startY, "SPEED");
    painter.drawText(startX+(spacingX*4+20), startY, "LAT");
    painter.drawText(startX+(spacingX*5+20), startY, "LONG");

    /// if no planes are in the list write No Planes
    if(airspace.size() == 0){
        QFont titleFont("Lato", 30, QFont::Bold);
        painter.setFont(titleFont);
        QString text("NO PLANES");
        QFontMetrics fm(titleFont);
        int advance = fm.horizontalAdvance(text);
        int xCenter = (pix.width() - advance) / 2;
        painter.drawText(xCenter, startY+spacingY*4, text);
    }
    startY += 10;

    /// Fill in the table
    int pasY = startY;
    painter.setFont(QFont(activeTheme->standardFont, 9));
    for (auto &entry : airspace) {
        QRect hex(startX-5,pasY,spacingX,spacingY);
        QRect callSign(startX+spacingX,pasY,spacingX+20,spacingY);
        QRect alt(startX+(spacingX*2+20),pasY,spacingX,spacingY);
        QRect spe(startX+(spacingX*3+20),pasY,spacingX,spacingY);
        QRect lat(startX+(spacingX*4+20),pasY,spacingX,spacingY);
        QRect lon(startX+(spacingX*5+20),pasY,spacingX,spacingY);

        painter.drawText(hex, 0, QString::number(entry.icao24, 16).rightJustified(6, '0').toUpper());
        painter.drawText(callSign, 0, entry.callSign);
        painter.drawText(alt, 0, entry.altValid ? QString::number(entry.alt) : "-");
        painter.drawText(spe, 0, entry.gndSpeedValid ? QString::number(entry.gndSpeed) : "-");
        painter.drawText(lat, 0, entry.latValid ? QString::number(entry.lat) : "-");
        painter.drawText(lon, 0, entry.lonValid ? QString::number(entry.lon) : "-");

        pasY += spacingY;

    }

    label->setPixmap(pix);
}

void FlightBoard::getAirTraffic() {
    airspace = WingletGUI::inst->adsbReceiver->airspace();
    auto reading = WingletGUI::inst->gpsReceiver->lastReading();
    if (reading.valid) {
        currentGPS = reading;
    }
    update();
}

} // namespace WingletUI
