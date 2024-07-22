#ifndef QCOMPASSMENUENTRY_H
#define QCOMPASSMENUENTRY_H


#include <mainwindow.h>

#include "AVQtWidgets/qrotarymenuentry.h"

class QCompassMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT


public:
    explicit QCompassMenuEntry(QWidget* parent, const QString& text);
    ~QCompassMenuEntry();

    void select();
    void set_return_to(QWidget* return_to);

private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
    QWidget* return_to;
    MainWindow* main_window;
};



#endif //QCOMPASSMENUENTRY_H
