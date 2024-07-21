#include "qsplashmenuentry.h"

#include <mainwindow.h>

#include "AVQtWidgets/splash.h"


QSplashMenuEntry::QSplashMenuEntry(QWidget *parent, const QString &text) : QRotaryMenuEntry(parent) {
    this->parent = parent;
    this->main_window = dynamic_cast<MainWindow*>(parent);
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QSplashMenuEntry::~QSplashMenuEntry() {
}

void QSplashMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}

void QSplashMenuEntry::select() {
    splash* splash_widget = new splash(this->parent);
    extern QWidget* primary_control;
    primary_control = splash_widget;
    this->main_window->set_active_controlled_widget(splash_widget);
    splash_widget->show();
}
