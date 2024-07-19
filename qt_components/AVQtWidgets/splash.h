#ifndef SPLASH_H
#define SPLASH_H

#include <QPaintEvent>
#include <QWidget>

//namespace Ui {
//class splash;
//}

class splash : public QWidget
{
    Q_OBJECT

public:
    explicit splash(QWidget *parent = nullptr);
    ~splash();
    void paintEvent(QPaintEvent *pEvent);
private:
//    Ui::splash *ui;
};

#endif // SPLASH_H
