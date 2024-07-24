#include "qcompassmenuentry.h"

#include <mainwindow.h>

#include "AVQtWidgets/compass.h"


QCompassMenuEntry::QCompassMenuEntry(QWidget *parent, const QString &text) : QRotaryMenuEntry(parent) {
    this->parent = parent;
    this->main_window = dynamic_cast<MainWindow*>(parent);
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QCompassMenuEntry::~QCompassMenuEntry() {
}

void QCompassMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}

void QCompassMenuEntry::select() {
    compass* compass_widget = new compass(this->parent);
    extern QWidget* primary_control;
    primary_control = compass_widget;
    this->main_window->set_active_controlled_widget(compass_widget);
    this->main_window->installEventFilter(compass_widget);
    compass_widget->show();
}
