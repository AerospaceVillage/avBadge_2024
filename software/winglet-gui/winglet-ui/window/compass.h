#ifndef COMPASS_H
#define COMPASS_H

#include <QWidget>
#include <QLabel>

namespace WingletUI {

class Compass : public QWidget
{
    Q_OBJECT
public:
    explicit Compass(QWidget *parent = nullptr);
    ~Compass();

protected:
    void hideEvent(QHideEvent *ev) override;
    void paintEvent(QPaintEvent *pEvent) override;
    void wheelEvent(QWheelEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private:
    void setLed(int idx, bool enable);

    QLabel *label;
    bool planePointer = false;
    double b = 0;
};

} // namespace WingletUI

#endif // COMPASS_H
