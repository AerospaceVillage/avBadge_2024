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

    void set_return_to(QWidget* return_to);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    private slots:
        void onQPress();
    void showTime();
private:
    Ui::Clock *ui;
    QWidget* parent;
    QWidget* return_to;
};

#endif // CLOCK_H
