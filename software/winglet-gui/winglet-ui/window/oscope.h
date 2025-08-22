#ifndef OSCOPE_H
#define OSCOPE_H

#include <QWidget>
#include <QPointF>
#include <unistd.h>
#include <QPushButton>
#include <QElapsedTimer>

#define MAX_POINTS 480
#define MAX_BUTTONS 6

#define DEFAULT_ZOOM 0.0f
#define MAX_ZOOM 2.0f
#define MIN_ZOOM -2.0f
#define ZOOM_DELTA 0.1f
#define ZOOM_FACTOR 10.0f

namespace WingletUI {

class OScope : public QWidget
{
    Q_OBJECT
public:
    explicit OScope(QWidget *parent = nullptr);

    enum ControlMode {
        OscilloscopeFocus = 0,
        WidgetFocus = 1,
        ButtonFocus = 2
    };

    enum SelectedButtonMode {
        None = -1,
        Waveform = 0,
        HorizontalZoom = 1,
        VerticalZoom = 2,
        TriggerRolling = 3,
        Color = 4,
        LineType = 5,
        MaxButtons = 6
    };

    enum WaveformState {
        Sin = 0,
        AnalogSquare = 1,
        DigitalSquare = 2,
        DigitalSawtooth = 3,
        DigitalTriangle = 4,
        MaxWaveforms = 5
    };

protected:
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void paintEvent(QPaintEvent *pEvent) override;
    void wheelEvent(QWheelEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void moveInX();

private:
    void startLED();
    void drawSin(QPainter* painter, QColor color);
    void drawAnalogSquare(QPainter* painter, QColor color);
    void drawDigitalSquare(QPainter* painter, QColor color);
    void drawDigitalTriangle(QPainter* painter, QColor color);
    void drawDigitalSawtooth(QPainter* painter, QColor color);
    void drawAnnularSector(QPainter* painter, const QPointF& center, qreal innerRadius, 
                        qreal outerRadius, qreal startAngle, qreal spanAngle, 
                        SelectedButtonMode buttonmode);
    void drawNeonLines(QPainter *painter, QColor color);
    void handleHorizontalZoom(bool scrollUp);
    void handleVerticalZoom(bool scrollUp);

    int getFontSizeForMode(SelectedButtonMode buttonMode) const;
    QString getButtonlabel(SelectedButtonMode buttonMode) const;
    QString getWaveformLabel() const;
    void updateWaveState(bool next);
    void updateButtonModeState(bool next);

    const QColor BG_COLOR = QColor(0,180,0);
    float amp = 100.;// in pixel
    float fre = .1;// in millisec
    float x = 0.;
    float y = 0.;
    QPointF points[MAX_POINTS];
    WaveformState state = Sin;
    bool image = false;
    int upDown = 240;

    bool phaseActive = false;
    float phase = 0;
    float phasePerFrame = 0.1f; // Adjust this value to control the speed of the phase change

    float horizontalZoom;
    float horizontalTimeScale;
    float verticalZoom;
    float verticalTimeScale;

    QPushButton* buttons[MAX_BUTTONS];
    SelectedButtonMode selectedButton = None;
    private:
    QElapsedTimer keyCooldownTimer;
    const int cooldownMs = 50;

    ControlMode controlMode = OscilloscopeFocus;
};

} // namespace WingletUI

#endif // OSCOPE_H
