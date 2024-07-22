#ifndef QOSCOPEMENUENTRY_H
#define QOSCOPEMENUENTRY_H


#include <mainwindow.h>

#include "AVQtWidgets/qrotarymenuentry.h"

class QOscopeMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT


public:
    explicit QOscopeMenuEntry(QWidget* parent, const QString& text);
    ~QOscopeMenuEntry();

    void select();
    void set_return_to(QWidget* return_to);

private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
    QWidget* return_to;
    MainWindow* main_window;
};



#endif //QOSCOPEMENUENTRY_H
