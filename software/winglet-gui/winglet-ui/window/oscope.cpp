#include "oscope.h"
#include "winglet-ui/theme.h"
#include "wingletgui.h"
#include "winglet-ui/widget/scrollablemenu.h"

#include <QTimer>
#include <QPainter>
#include <cmath>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QPainterPath>
#include <QtMath>

#define WAVE_SIN_LABEL "WAVE: Sin"
#define WAVE_ANALOG_SQUARE_LABEL "WAVE: AnaSqr"
#define WAVE_DIGITAL_SQUARE_LABEL "WAVE: DigSqr"
#define WAVE_DIGITAL_SAWTOOTH_LABEL "WAVE: Sawtooth"
#define WAVE_DIGITAL_TRIANGLE_LABEL "WAVE: Triangle"

#define BUTTON_LABEL_WAVEFORM        "Waveform"
#define BUTTON_LABEL_VERTICAL_ZOOM   "Vertical Zoom"
#define BUTTON_LABEL_HORIZONTAL_ZOOM "Horizontal Zoom"
#define BUTTON_LABEL_LINE_TYPE       "Line Type"
#define BUTTON_LABEL_TRIGGER_ROLLING "Trigger/Rolling"
#define BUTTON_LABEL_COLOR           "Color"

#define HORIZONTAL_ZOOM_LABEL "H Zoom: "
#define VERTICAL_ZOOM_LABEL   "V Zoom: "
#define LABEL_TRIGGER_ROLLING "Trigger/Rolling"
#define LABEL_COLOR           "Color"
#define LABEL_LINE_TYPE       "Line Type"

#define CENTER_X 240
#define CENTER_Y 240
#define RADIUS 240

#define NOISE_LEVEL_X 0.3f
#define NOISE_LEVEL_Y 0.5f
#define DRAW_GLOW_LAYERS 4
#define DRAW_GLOW_WIDTH 4.0f

namespace WingletUI {
static bool debug = false;
static u_int32_t seed = 0;
static uint32_t wangHash(uint32_t key) {
    key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * 0x27d4eb2d;
    key = key ^ (key >> 15);
    return key;
}
static double normalizeSeed(uint32_t h) {
    return (static_cast<double>(h / RAND_MAX) * 2.0 - 1.0);
}

OScope::OScope(QWidget *parent)
    : QWidget{parent}
{
    this->setGeometry(0, 0, 480, 480);

    // QTimer* tir = new QTimer(this);
    // connect(tir, SIGNAL(timeout()), this, SLOT(moveInX()));
    // tir->start(10);

    horizontalZoom = DEFAULT_ZOOM;
    horizontalTimeScale = pow(ZOOM_FACTOR, horizontalZoom);
    verticalZoom = DEFAULT_ZOOM;
    verticalTimeScale = pow(ZOOM_FACTOR, verticalZoom);

    QWidget* infoBar = new QWidget(this);
    const char* labels[MAX_BUTTONS] = {
        BUTTON_LABEL_WAVEFORM,
        BUTTON_LABEL_VERTICAL_ZOOM,
        BUTTON_LABEL_HORIZONTAL_ZOOM,
        BUTTON_LABEL_LINE_TYPE,
        BUTTON_LABEL_TRIGGER_ROLLING,
        BUTTON_LABEL_COLOR,
    };

    for (int i = 0; i < MAX_BUTTONS; ++i) {
        buttons[i] = new QPushButton(infoBar);
        buttons[i]->setObjectName(QString(labels[i]));
        buttons[i]->setVisible(false);

        // connect(buttons[i], &QPushButton::clicked, this, [this]() {
        //     qDebug() << QString("%1").arg(sender()->objectName()) << "button clicked";
        // });
    }

    keyCooldownTimer.start();
}

void OScope::startLED(){
    // rgb incolor;
    // incolor.r = 0.0;
    // incolor.g = 0.3;
    // incolor.b = 0.0;

    for (int idx = 0; idx < 20; idx++) {
        WingletGUI::inst->ledControl->setRingLed(idx, 0, 85, 0);
    }
}

void OScope::showEvent(QShowEvent *ev)
{
    (void) ev;
    startLED();
}

void OScope::hideEvent(QHideEvent *ev)
{
    (void) ev;
    WingletGUI::inst->ledControl->clearRing();
}

void OScope::paintEvent(QPaintEvent *pEvent){
    (void) pEvent;

    QPainter* painter = new QPainter(this);
    QColor bg = activeTheme->palette.color(QPalette::Window);
    painter->setBrush(bg);
    painter->drawRect(0,0,480,480);

    //// draw Oscillosocpe grid
    QColor grid(255,255,255);
    QPen pen(grid);
    pen.setWidth(1);
    painter->setPen(pen);
    int divNum = 6;
    for(int i = 0 ; i<divNum; i++){
    /// draw verical lines
       painter->drawLine((480/divNum)*i,0,(480/divNum)*i,480);
    /// draw horizonal lines
       painter->drawLine(0,(480/divNum)*i,480,(480/divNum)*i);
    }

    QFont bold(activeTheme->standardFont, 14);
    bold.setBold(true);
    painter->setFont(bold);
    QString sta;
    QString amp("Amplitude: "+QString::number(this->amp));
    QString per("Period: "+QString::number(this->fre));
    QString hZoom("Horizontal Zoom: " + QString::number(this->horizontalZoom));
    QString vZoom("Vertical Zoom: " + QString::number(this->verticalZoom));

    QColor ball = *activeTheme->oscope_line_color;
    if(this->state == Sin){
        drawSin(painter, ball);
        sta = "WAVE: Sin";
    }
    else if(this->state == AnalogSquare){
        drawAnalogSquare(painter, ball);
        sta = "WAVE: AnaSqr";
    }
    else if(this->state == DigitalSquare){
        drawDigitalSquare(painter, ball);
        sta = "WAVE: DigSqr";
    }
    else if(this->state == DigitalSawtooth){
        drawDigitalSawtooth(painter, ball);
        sta = "WAVE: Sawtooth";
    }
    else if(this->state == DigitalTriangle){
        drawDigitalTriangle(painter, ball);
        sta = "WAVE: Triangle";
    }
    // Use theme palette for text
    if (debug) {
        QColor textColor = activeTheme->palette.color(QPalette::WindowText);
        painter->setPen(textColor);
        painter->drawText(185,20, sta);
        painter->drawText(200,400, amp);
        painter->drawText(200,420, per);
        painter->drawText(200,440, hZoom);
        painter->drawText(200,460, vZoom);
    }

    // image
    if (image == true){
        QRect size(90,90,300,300);

        QImage img(":/images/FrogFly.png");
        painter->drawImage(size,img);
    }

    if (phaseActive) {
        phase += phasePerFrame;
        if (phase > 256 * M_PI) phase -= 256 * M_PI;
    }

    const int innerRadius = RADIUS - 50;
    const int outerRadius = RADIUS;
    drawAnnularSector(painter, QPointF(CENTER_X, CENTER_Y), innerRadius, outerRadius, 105, 30, Waveform);
    drawAnnularSector(painter, QPointF(CENTER_X, CENTER_Y), innerRadius, outerRadius, 75, 30, HorizontalZoom);
    drawAnnularSector(painter, QPointF(CENTER_X, CENTER_Y), innerRadius, outerRadius, 45, 30, VerticalZoom);

    drawAnnularSector(painter, QPointF(CENTER_X, CENTER_Y), innerRadius, outerRadius, 45 + 180, 30, TriggerRolling);
    drawAnnularSector(painter, QPointF(CENTER_X, CENTER_Y), innerRadius, outerRadius, 75 + 180, 30, Color);
    drawAnnularSector(painter, QPointF(CENTER_X, CENTER_Y), innerRadius, outerRadius, 105 + 180, 30, LineType);
    delete painter;
}

void OScope::drawAnnularSector(QPainter* painter, const QPointF& center, qreal innerRadius, 
                      qreal outerRadius, qreal startAngle, qreal spanAngle, 
                      SelectedButtonMode buttonMode) {
    QColor hoveredSectorColor = activeTheme->palette.color(QPalette::Highlight);
    hoveredSectorColor.setAlpha(180);

    QColor sectorColor = activeTheme->palette.color(QPalette::Midlight);
    sectorColor.setAlpha(128);

    QColor selectedColor = activeTheme->palette.color(QPalette::Link);
    selectedColor.setAlpha(180);

    QColor fillColor;
    if (selectedButton == buttonMode && controlMode == ButtonFocus) {
        fillColor = selectedColor;
    } else if (selectedButton == buttonMode) {
        fillColor = hoveredSectorColor;
    } else {
        fillColor = sectorColor;
    }
    QPainterPath sectorPath;
    sectorPath.moveTo(center);
    sectorPath.arcTo(QRectF(0, 0, RADIUS * 2, RADIUS * 2),
                    startAngle, spanAngle);
    sectorPath.closeSubpath();

    QPainterPath circlePath;
    circlePath.addEllipse(center, innerRadius, innerRadius);

    painter->save();
    painter->setBrush(QBrush(fillColor));
    QColor penColor = activeTheme->palette.color(QPalette::WindowText);
    painter->setPen(QPen(penColor, 2));
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawPath(sectorPath.subtracted(circlePath));
    painter->restore();

    painter->save();
    int fontSize = getFontSizeForMode(buttonMode);
    QFont font = QFont(activeTheme->standardFont, fontSize);
    QColor textColor = activeTheme->palette.color(QPalette::WindowText);
    painter->setPen(textColor);
    painter->setFont(font);
    QFontMetrics fm(font);
    QString str = getButtonlabel(buttonMode);
    int width=fm.horizontalAdvance(str);
    painter->translate(center);

    if (startAngle >= 0 && startAngle <= 180) {
        painter->rotate(-startAngle + 90 - spanAngle / 2);
    } else {
        painter->rotate(-(startAngle + spanAngle / 2 - 270));
    }
    painter->translate(-width/2, 0);
    if (startAngle <= 180 && startAngle >= 0) {
        painter->translate(0, -innerRadius - (outerRadius - innerRadius) / 4);
    } else {
        painter->translate(0, (innerRadius + (outerRadius - innerRadius) / 2));
    }
    painter->drawText(0, 0, str);
    painter->restore();
}

int OScope::getFontSizeForMode(SelectedButtonMode buttonMode) const {
    switch (buttonMode) {
        case Waveform:
            return state == DigitalSawtooth ? 10 : 12;
        case HorizontalZoom:
        case VerticalZoom:
        case TriggerRolling:
        case Color:
        case LineType:
            return 12;
        case None:
        case MaxButtons:
            return 0;
    }
    return 0;
}

QString OScope::getButtonlabel(SelectedButtonMode buttonMode) const {
    switch (buttonMode) {
        case Waveform: return getWaveformLabel();
        case HorizontalZoom: return QString(HORIZONTAL_ZOOM_LABEL + QString::number(this->horizontalZoom, 'f', 1));
        case VerticalZoom: return QString(VERTICAL_ZOOM_LABEL + QString::number(this->verticalZoom, 'f', 1));
        case TriggerRolling: return LABEL_TRIGGER_ROLLING;
        case Color: return LABEL_COLOR;
        case LineType: return LABEL_LINE_TYPE;
        case None:
        case MaxButtons:
            break;
    }
    return QString("");
}
QString OScope::getWaveformLabel() const {
    switch (state) {
        case Sin: return WAVE_SIN_LABEL;
        case AnalogSquare: return WAVE_ANALOG_SQUARE_LABEL;
        case DigitalSquare: return WAVE_DIGITAL_SQUARE_LABEL;
        case DigitalSawtooth: return WAVE_DIGITAL_SAWTOOTH_LABEL;
        case DigitalTriangle: return WAVE_DIGITAL_TRIANGLE_LABEL;
        case MaxWaveforms:
            break;
    }
    return QString("");
}

void OScope::drawNeonLines(QPainter *painter, QColor color)
{
    // Glow layers (draw from outside glow inward)
    // painter->setRenderHint(QPainter::Antialiasing); // Disabled for performance
    for (int i = 0; i < MAX_POINTS; ++i) {
        float noiseX = normalizeSeed(wangHash(this->points[i].x() + seed)) * NOISE_LEVEL_X;
        float noiseY = normalizeSeed(wangHash(this->points[i].x() + seed + 1)) * NOISE_LEVEL_Y; // scale by frequency too?

        // Scale by zoom level around the center
        float x = CENTER_X + (this->points[i].x() + noiseX - CENTER_X) * horizontalTimeScale;
        float y = CENTER_Y + (this->points[i].y()  + noiseY - CENTER_Y) * verticalTimeScale;

        this->points[i] = QPointF(x, y);
        seed = (seed + 1) % UINT32_MAX;
    }

    for (int i = DRAW_GLOW_LAYERS; i >= 1; --i) {
        int alpha = 20 * (DRAW_GLOW_LAYERS + 1 - i);
        painter->save();
        color.setAlpha(alpha);
        QPen pen(color);
        pen.setWidth(i * DRAW_GLOW_WIDTH);
        pen.setCapStyle(Qt::SquareCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->drawPolyline(this->points, MAX_POINTS);
        painter->restore();
    }
}

void OScope::drawSin(QPainter *painter, QColor color) {
    for (int x = 0; x < MAX_POINTS; ++x) {
        float y = (this->amp * sin(this->fre * (x) + phase)) + upDown;
        this->points[x] = QPointF(x, y);
    }

    drawNeonLines(painter, color);
    update();
}

void OScope::drawAnalogSquare(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){
        this->points[i] = QPointF(x+float(i), (this->amp*sin(this->fre*(x+float(i)+phase)))+(upDown) +(this->amp*sin(this->fre*(3*(x+float(i)+phase)))/3)+(this->amp*sin(this->fre*(5*(x+float(i)+phase)))/5)+(this->amp*sin(this->fre*(7*(x+float(i)+phase)))/7));
    }
    // painter->drawPolyline(this->points, MAX_POINTS);
    drawNeonLines(painter, color);
    update();
}

void OScope::drawDigitalSquare(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){

        this->points[i] = QPointF(x+float(i), 1.5*this->amp*pow(0,pow(0,(sin(this->fre*(x+float(i))+phase))))+(upDown - 25 - this->amp/2));
    }
    // painter->drawPolyline(this->points, MAX_POINTS);
    drawNeonLines(painter, color);
    update();
}

void OScope::drawDigitalSawtooth(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){

        this->points[i] = QPointF(x+float(i), abs(fmod(this->fre*10*(x+float(i))+phase,this->amp*1.5) - this->amp*1.5) + (upDown - 25 - this->amp/2));
    }
    // painter->drawPolyline(this->points, MAX_POINTS);
    drawNeonLines(painter, color);
    update();
}

void OScope::drawDigitalTriangle(QPainter *painter, QColor color){
    QPen pen(color);
    pen.setWidth(4);
    painter->setBrush(QBrush(color));
    painter->setPen(pen);
    float x = this->x;
    for(int i = 0 ;i < MAX_POINTS;i++){

        this->points[i] = QPointF(x+float(i), (0.5)*this->amp*asin(sin(this->fre*(x+float(i)+phase))) + (upDown));
    }
    // painter->drawPolyline(this->points, MAX_POINTS);
    drawNeonLines(painter, color);
    update();
}

void OScope::moveInX(){
    if(this->x == 480){
       this->x = -50;
    }
    this->x = this->x+1;
}

void OScope::wheelEvent(QWheelEvent *ev) {
    const bool scrollUp = ev->angleDelta().y() > 0;
    if (controlMode == OscilloscopeFocus) {
        updateWaveState(scrollUp);
        update();
    } else if (controlMode == WidgetFocus) {
        updateButtonModeState(scrollUp);
        update();
    } else if (controlMode == ButtonFocus) {
        if (selectedButton == Waveform) {
            updateWaveState(scrollUp);
        } else if(selectedButton == HorizontalZoom) {
            handleHorizontalZoom(scrollUp);
        } else if(selectedButton == VerticalZoom) {
            handleVerticalZoom(scrollUp);
        }
        update();
    }
}

void OScope::updateWaveState(bool next) {
    if (next) {
        if(this->state >= 0 && this->state < (MaxWaveforms - 1)){
            this->state = static_cast<WaveformState>(this->state + 1);
        }
    } else {
        if(this->state > 0 && this->state <= (MaxWaveforms - 1)){
            this->state = static_cast<WaveformState>(this->state - 1);
        }
    }
    phase = 0;
}

void OScope::updateButtonModeState(bool next) {
    switch (selectedButton) {
        case Waveform:
            selectedButton = next ? HorizontalZoom : TriggerRolling;
            break;
        case HorizontalZoom:
            selectedButton = next ? VerticalZoom : Waveform;
            break;
        case VerticalZoom:
            selectedButton = next ? LineType : HorizontalZoom;
            break;
        case TriggerRolling:
            selectedButton = next ? Waveform : Color;
            break;
        case Color:
            selectedButton = next ? TriggerRolling : LineType;
            break;
        case LineType:
            selectedButton = next ? Color : VerticalZoom;
            break;
        case None:
        case MaxButtons:
            break;
    }
}

void OScope::handleHorizontalZoom(bool scrollUp) {
    if (scrollUp) {
        horizontalZoom += ZOOM_DELTA;
        if (horizontalZoom > MAX_ZOOM) {
            horizontalZoom = MAX_ZOOM; // Prevent zooming in too much
        }
    } else {
        horizontalZoom -= ZOOM_DELTA;
        if (horizontalZoom < MIN_ZOOM) {
            horizontalZoom = MIN_ZOOM; // Prevent zooming out too much
        }
    }

    horizontalTimeScale = pow(ZOOM_FACTOR, horizontalZoom);
}

void OScope::handleVerticalZoom(bool scrollUp) {
    if (scrollUp) {
        verticalZoom += ZOOM_DELTA;
        if (verticalZoom > MAX_ZOOM) {
            verticalZoom = MAX_ZOOM; // Prevent zooming in too much
        }
    } else {
        verticalZoom -= ZOOM_DELTA;
        if (verticalZoom < MIN_ZOOM) {
            verticalZoom = MIN_ZOOM; // Prevent zooming out too much
        }
    }
    verticalTimeScale = pow(ZOOM_FACTOR, verticalZoom);
}

void OScope::keyPressEvent(QKeyEvent *ev) {
    if (controlMode == OscilloscopeFocus) {
        switch (ev->key()) {
        case Qt::Key_Up: {
            if(this->amp >= 10 && this->amp < 200){
                this->amp += 1;
            }
            update();
            break;
        }
        case Qt::Key_Down: {
            if(this->amp > 10 && this->amp <= 200){
                this->amp -=1;
            }
            update();
            break;
        }
        case Qt::Key_Right: {
            if (keyCooldownTimer.elapsed() < cooldownMs)
                break;
            keyCooldownTimer.restart();
            if(this->fre >= .01 && this->fre < .79){
                this->fre += .01;
            }
            update();
            break;
        }
        case Qt::Key_Left: {
            if (keyCooldownTimer.elapsed() < cooldownMs)
                break;
            keyCooldownTimer.restart();
            if(this->fre > .02 && this->fre <= .8 ){
                this->fre -= .01;
            }
            update();
            break;
        }
        case Qt::Key_Return:
        case Qt::Key_A: {
            selectedButton = Waveform;
            controlMode = WidgetFocus;
            update();
            break;
        }
        default:
            ev->ignore();
        }
    } else if (controlMode == WidgetFocus) {
        switch (ev->key()) {
        case Qt::Key_Up: {
            update();
            break;
        }
        case Qt::Key_Down: {
            update();
            break;
        }
        case Qt::Key_Right: {
            update();
            break;
        }
        case Qt::Key_Left: {
            update();
            break;
        }
        case Qt::Key_Return:
        case Qt::Key_A: {
            controlMode = ButtonFocus;
            update();
            break;
        }
        case Qt::Key_B: {
            selectedButton = None;
            controlMode = OscilloscopeFocus;
            update();
            break;
        }
        default:
            ev->ignore();
        }
    } else if (controlMode == ButtonFocus) {
        switch (ev->key()) {
        case Qt::Key_Up: {
            update();
            break;
        }
        case Qt::Key_Down: {
            update();
            break;
        }
        case Qt::Key_Right: {
            if (selectedButton == Waveform) {
                updateWaveState(true);
            } else if (selectedButton == HorizontalZoom) {
                handleHorizontalZoom(true);
            } else if (selectedButton == VerticalZoom) {
                handleVerticalZoom(true);
            }
            update();
            break;
        }
        case Qt::Key_Left: {
            if (selectedButton == Waveform) {
                updateWaveState(false);
            } else if (selectedButton == HorizontalZoom) {
                handleHorizontalZoom(false);
            } else if (selectedButton == VerticalZoom) {
                handleVerticalZoom(false);
            }
            update();
            break;
        }
        case Qt::Key_Return:
        case Qt::Key_A: {
            if (selectedButton == TriggerRolling) {
                phaseActive = !phaseActive;
            }
            update();
            break;
        }
        case Qt::Key_B: {
            controlMode = WidgetFocus;
            update();
            break;
        }
        default:
            ev->ignore();
        }
    }
}

} // namespace WingletUI
