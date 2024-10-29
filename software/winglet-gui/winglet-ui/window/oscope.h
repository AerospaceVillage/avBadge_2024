#ifndef OSCOPE_H
#define OSCOPE_H

#include <QWidget>
#include <QPointF>

#define  MAX_POINTS 100

namespace WingletUI {

class OScope : public QWidget
{
    Q_OBJECT
public:
    explicit OScope(QWidget *parent = nullptr);

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

    const QColor BG_COLOR = QColor(0,180,0);
    float amp = 100.;// in pixel
    float fre = .1;// in millisec
    float x = 0.;
    float y = 0.;
    QPointF points[MAX_POINTS];
    int state = 0;
    bool image = false;
    int upDown = 240;
};

} // namespace WingletUI

#endif // OSCOPE_H
