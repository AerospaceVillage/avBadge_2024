#include "qmediaplayermenuentry.h"

#include <mainwindow.h>

#include "AVQtWidgets/simplemediaplayer.h"


QMediaPlayerMenuEntry::QMediaPlayerMenuEntry(QWidget *parent, const QString &text) : QRotaryMenuEntry(parent) {
    this->parent = parent;
    this->main_window = dynamic_cast<MainWindow*>(parent);
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QMediaPlayerMenuEntry::~QMediaPlayerMenuEntry() {
}

void QMediaPlayerMenuEntry::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}

void QMediaPlayerMenuEntry::select() {
    simplemediaplayer* mediaplayer_widget = new simplemediaplayer(this->parent);
    extern QWidget* primary_control;
    primary_control = mediaplayer_widget;
    this->main_window->set_active_controlled_widget(mediaplayer_widget);
    mediaplayer_widget->show();
}
