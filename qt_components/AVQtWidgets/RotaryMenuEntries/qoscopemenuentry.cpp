#include "qoscopemenuentry.h"

#include <mainwindow.h>

#include "AVQtWidgets/oscope.h"


QOscopeMenuEntry::QOscopeMenuEntry(QWidget *parent, const QString &text) : QRotaryMenuEntry(parent) {
    this->parent = parent;
    this->main_window = dynamic_cast<MainWindow*>(parent);
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QOscopeMenuEntry::~QOscopeMenuEntry() {
}

void QOscopeMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}

void QOscopeMenuEntry::select() {
    oScope* oscope_widget = new oScope(this->parent);
    extern QWidget* primary_control;
    primary_control = oscope_widget;
    this->main_window->set_active_controlled_widget(oscope_widget);
    this->main_window->installEventFilter(oscope_widget);
    oscope_widget->show();
}

