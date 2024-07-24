#ifndef QRADARSCOPEMENUENTRY_H
#define QRADARSCOPEMENUENTRY_H


#include <mainwindow.h>

#include "AVQtWidgets/qrotarymenuentry.h"

class QRadarScopeMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT


public:
    explicit QRadarScopeMenuEntry(QWidget* parent, const QString& text);
    ~QRadarScopeMenuEntry();

    void select();
    void set_return_to(QWidget* return_to);

private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
    QWidget* return_to;
    MainWindow* main_window;
};



#endif //QRADARSCOPEMENUENTRY_H
