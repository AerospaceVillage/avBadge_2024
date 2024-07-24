#ifndef COMPASS_H
#define COMPASS_H

#include <QKeyEvent>
#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>




namespace Ui {
class compass;
}

class compass : public QWidget
{
    Q_OBJECT

public:
    explicit compass(QWidget *parent = nullptr);
    ~compass();

    bool planePointer = false;
    void paintEvent(QPaintEvent *pEvent);
    void rotate();
    double b;

protected:
    bool eventFilter(QObject *obj, QEvent *event);


protected:



private:
    Ui::compass *ui;
};

#endif // COMPASS_H
