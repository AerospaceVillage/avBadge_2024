#ifndef QFLIGHTBOARDMENUENTRY_H
#define QFLIGHTBOARDMENUENTRY_H


#include <mainwindow.h>

#include "AVQtWidgets/qrotarymenuentry.h"

class QFlightBoardMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT


public:
    explicit QFlightBoardMenuEntry(QWidget* parent, const QString& text);
    ~QFlightBoardMenuEntry();

    void select();
    void set_return_to(QWidget* return_to);
    QWidget* actualWidget;
private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
    QWidget* return_to;
    MainWindow* main_window;
};



#endif //QFLIGHTBOARDMENUENTRY_H
