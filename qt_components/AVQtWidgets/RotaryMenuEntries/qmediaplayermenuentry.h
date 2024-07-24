#ifndef QMEDIAPLAYERMENUENTRY_H
#define QMEDIAPLAYERMENUENTRY_H


#include <mainwindow.h>

#include "AVQtWidgets/qrotarymenuentry.h"

class QMediaPlayerMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT


public:
    explicit QMediaPlayerMenuEntry(QWidget* parent, const QString& text);
    ~QMediaPlayerMenuEntry();

    void select();
    void set_return_to(QWidget* return_to);

private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
    QWidget* return_to;
    MainWindow* main_window;
};



#endif //QMEDIAPLAYERMENUENTRY_H
