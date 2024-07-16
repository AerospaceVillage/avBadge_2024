#ifndef QMENUENTRY_H
#define QMENUENTRY_H

#include <QLabel>
#include <QEvent>

class QRotaryMenuEntry: public QLabel {
    Q_OBJECT

public:
    explicit QRotaryMenuEntry(const QString& text, QWidget* parent = nullptr, QWidget* widget = nullptr);
    ~QRotaryMenuEntry(); // TODO: Ensure all destructors fire.

    void select();


private:
    QWidget* widget;
};



#endif //QMENUENTRY_H
