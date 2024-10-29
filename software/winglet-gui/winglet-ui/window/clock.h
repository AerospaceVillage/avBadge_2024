#ifndef WINGLETUI_CLOCK_H
#define WINGLETUI_CLOCK_H

#include <QWidget>

class QLabel;
class QPropertyAnimation;

namespace WingletUI {

class Clock : public QWidget
{
    Q_OBJECT

public:
    explicit Clock(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void showTime();
    void animationFinished();

private:
    void beginHide();

    QPropertyAnimation *fadeAnimation;
    QLabel *clockLabel;
    QLabel *avLogoLabel;

    bool animationIsHiding;
    bool clockShowSeconds;
};

} // namespace WingletUI

#endif // WINGLETUI_CLOCK_H
