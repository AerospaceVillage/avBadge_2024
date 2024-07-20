#include "qclockmenuentry.h"
#include "AVQtWidgets/clock.h"


QClockMenuEntry::QClockMenuEntry(QWidget *parent, const QString &text) : QRotaryMenuEntry(parent) {
    this->parent = parent;
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QClockMenuEntry::~QClockMenuEntry() {
}

void QClockMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}


void QClockMenuEntry::select() {
    Clock* clockW = new Clock(this->parent);
    extern QWidget* primary_control;
    primary_control = clockW;
    this->parent->installEventFilter(clockW);
    clockW->set_return_to(this->return_to);
    clockW->show();
}
