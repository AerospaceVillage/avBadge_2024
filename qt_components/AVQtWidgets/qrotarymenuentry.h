#ifndef QMENUENTRY_H
#define QMENUENTRY_H

#include <QLabel>
#include <QEvent>

class QRotaryMenuEntry: public QLabel {
    Q_OBJECT

public:
    explicit QRotaryMenuEntry(QWidget* parent);
    ~QRotaryMenuEntry();

    virtual void select() = 0;

private:
    QWidget* parent;
};



#endif //QMENUENTRY_H
