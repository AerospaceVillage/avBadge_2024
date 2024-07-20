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
    for (int i=0; i<active_widgets.length(); i++) {
        delete active_widgets[i];
    }
}

void QClockMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}


void QClockMenuEntry::select() {
    Clock* clockW = new Clock(this->parent);
    extern QWidget* primary_control;
    primary_control = clockW;
    active_widgets.append(clockW);
    this->parent->installEventFilter(clockW);
    clockW->set_return_to(this->return_to);
    clockW->show();
}
