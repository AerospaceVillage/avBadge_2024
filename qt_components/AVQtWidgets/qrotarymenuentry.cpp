#include "qrotarymenuentry.h"

#include <qmenu.h>
#include <QEvent>


QRotaryMenuEntry::QRotaryMenuEntry(const QString& text, QWidget* parent, QWidget* widget) : QLabel(parent)  {
    this->widget = widget;
    this->setText(text);
    this->setAlignment(Qt::AlignLeft);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->setStyleSheet("font-size: 16px; color: white;");
    this->installEventFilter(this);
}

QRotaryMenuEntry::~QRotaryMenuEntry() {}

void QRotaryMenuEntry::select() {
}

