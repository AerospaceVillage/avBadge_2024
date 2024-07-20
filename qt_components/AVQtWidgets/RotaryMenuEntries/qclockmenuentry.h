#ifndef QCLOCKMENUENTRY_H
#define QCLOCKMENUENTRY_H

#include "AVQtWidgets/qrotarymenuentry.h"

class QClockMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT


public:
    explicit QClockMenuEntry(QWidget* parent, const QString& text);
    ~QClockMenuEntry();

    void select();
    void set_return_to(QWidget* return_to);



private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
    QWidget* return_to;
};

#endif //QCLOCKMENUENTRY_H
