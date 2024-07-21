#include "qflightboardmenuentry.h"

#include <mainwindow.h>

#include "AVQtWidgets/flightboard.h"


QFlightBoardMenuEntry::QFlightBoardMenuEntry(QWidget *parent, const QString &text) : QRotaryMenuEntry(parent) {
    this->parent = parent;
    this->main_window = dynamic_cast<MainWindow*>(parent);
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QFlightBoardMenuEntry::~QFlightBoardMenuEntry() {
}

void QFlightBoardMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}

void QFlightBoardMenuEntry::select() {
    flightBoard* flightboard_widget = new flightBoard(this->parent);
    this->actualWidget = flightboard_widget;
    extern QWidget* primary_control;
    primary_control = flightboard_widget;
    this->main_window->set_active_controlled_widget(flightboard_widget);
    flightboard_widget->show();
}
