#ifndef QSPLASHMENUENTRY_H
#define QSPLASHMENUENTRY_H


#include <mainwindow.h>

#include "AVQtWidgets/qrotarymenuentry.h"

class QSplashMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT


public:
    explicit QSplashMenuEntry(QWidget* parent, const QString& text);
    ~QSplashMenuEntry();

    void select();
    void set_return_to(QWidget* return_to);

private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
    QWidget* return_to;
    MainWindow* main_window;
};



#endif //QSPLASHMENUENTRY_H
