#include "qtestmenuentry.h"


QTestMenuEntry::QTestMenuEntry(QWidget* parent, const QString &text) : QRotaryMenuEntry(parent){
    this->parent = parent;
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->installEventFilter(this);
}

QTestMenuEntry::~QTestMenuEntry() {
    for (int i=0; i<active_widgets.length(); i++) {
        delete active_widgets[i];
    }
}

void QTestMenuEntry::select() {
    QWidget* test_widget = new QWidget(this->parent);
    active_widgets.append(test_widget);
    test_widget->show();
}

