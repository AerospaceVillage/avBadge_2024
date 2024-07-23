#include "qradarscopemenuentry.h"

#include <mainwindow.h>

#include "AVQtWidgets/radarscope.h"


QRadarScopeMenuEntry::QRadarScopeMenuEntry(QWidget *parent, const QString &text) : QRotaryMenuEntry(parent) {
    this->parent = parent;
    this->main_window = dynamic_cast<MainWindow*>(parent);
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QRadarScopeMenuEntry::~QRadarScopeMenuEntry() {
}

void QRadarScopeMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}

void QRadarScopeMenuEntry::select() {
    radarscope* radarscope_widget = new radarscope(this->parent);
    extern QWidget* primary_control;
    primary_control = radarscope_widget;
    this->main_window->set_active_controlled_widget(radarscope_widget);
    radarscope_widget->show();
}
