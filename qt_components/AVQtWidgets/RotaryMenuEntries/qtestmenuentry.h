#ifndef QTESTMENUENTRY_H
#define QTESTMENUENTRY_H


#include "AVQtWidgets/qrotarymenuentry.h"


class QTestMenuEntry: public QRotaryMenuEntry {
    Q_OBJECT

public:
    explicit QTestMenuEntry(QWidget* parent, const QString& text);
    ~QTestMenuEntry();

    void select();

private:
    QWidget* parent;
    QVector<QWidget*> active_widgets;
};

#endif //QTESTMENUENTRY_H
