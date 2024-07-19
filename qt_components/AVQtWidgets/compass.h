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
    virtual void paintEvent(QPaintEvent *pEvent);
    void keyPressEvent(QKeyEvent *event);
    void rotate();
    double b;
    void wheelEvent(QWheelEvent* event);


protected:



private:
    Ui::compass *ui;
};

#endif // COMPASS_H
