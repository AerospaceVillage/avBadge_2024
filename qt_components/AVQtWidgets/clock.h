#ifndef CLOCK_H
#define CLOCK_H

#include <QWidget>

namespace Ui {
class Clock;
}

class Clock : public QWidget
{
    Q_OBJECT

public:
    explicit Clock(QWidget *parent = nullptr);
    ~Clock();

    void showTime();
private:
    Ui::Clock *ui;
};

#endif // CLOCK_H
