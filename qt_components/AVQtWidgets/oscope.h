#ifndef OSCOPE_H
#define OSCOPE_H

#include <QWidget>
#include <QPointF>
#define  MAX_POINTS 42
// namespace Ui {
// class oScope;
// }

class oScope : public QWidget
{
    Q_OBJECT

public:
    explicit oScope(QWidget *parent = nullptr);
    ~oScope();
    QPainter *painter;
    float amp = 100.;// in pixel
    float fre = .1;// in millisec
    float x = 0.;
    float y = 0.;
    int maxPoints = MAX_POINTS;
    QPointF points[MAX_POINTS];
    int state = 0;
    bool image = false;
    int upDown = 240;

    QColor BG_COLOR = QColor(0,180,0);

    void paintEvent(QPaintEvent *pEvent);
    void moveInX();
    void drawSin(QColor color);
    void drawAnalogSquare(QColor color);
    void drawDigitalSquare(QColor color);
    void drawDigitalTriangle(QColor color);
    void drawDigitalSawtooth(QColor color);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    // Ui::oScope *ui;
};

#endif // OSCOPE_H
