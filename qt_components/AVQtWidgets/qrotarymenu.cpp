#include "qrotarymenu.h"
#include <QKeyEvent>
#include <algorithm>


int QRotaryMenu::wrapIndex(int size, int index) {
    return (index % size + size) % size;
}

void QRotaryMenu::redrawEntries(int lower_index, int element_length) {
    QFont font;
    font.setFamily(QString::fromUtf8("Neuropol X"));
    font.setPointSize(8);

    for (int i = 0; i < entries.length(); i++) {
        entries[i]->hide();
        entries[i]->setFont(font);
    }

    int vec_size = this->entries.size();
    for (int i=lower_index; i<(lower_index+element_length); i++) {
        int wrapped_index = wrapIndex(vec_size, i);
        if (i == lower_index + 2) {
            entries[wrapped_index]->setGeometry(QRect(50, 60+((i-lower_index)*50-7), 200, 100)); // TODO: Make it a curve.
        }
        else {
            entries[wrapped_index]->setGeometry(QRect(50, 60+((i-lower_index)*50), 200, 100)); // TODO: Make it a curve.
        }
        entries[wrapped_index]->show();
    }
}

QRotaryMenu::QRotaryMenu(QList<QRotaryMenuEntry*> entries, QWidget *parent, QString text, QString highlight_text) : QWidget(parent) {
    this->entries = entries;
    this->text = text;
    this->highlight_text = highlight_text;
    this->parent = parent;

    QWidget* centralwidget = new QWidget(parent);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    centralwidget->setStyleSheet("background-color:rgb(0);");

    int vec_size = this->entries.size();
    int lower_index = wrapIndex(vec_size, this->selected_entry_index-2-1);
    int upper_index = wrapIndex(vec_size, this->selected_entry_index+2-1);
    int elements_length = std::min(vec_size - std::abs(upper_index - lower_index), std::abs(upper_index - lower_index))+1;
    redrawEntries(lower_index, elements_length+1);
    // TODO: Setup.
    this->selected_entry_index = 2;
    this->selected_entry = entries[this->selected_entry_index];
    this->selected_entry->setStyleSheet(this->highlight_text);

}

QRotaryMenu::~QRotaryMenu() {}

bool QRotaryMenu::eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
                case Qt::Key_Up: {
                    onEntrySelectionUp();
                    break;
                }
                case Qt::Key_Down: {
                    onEntrySelectionDown();
                    break;
                }
                default:
                    break;
            }
        }
        return obj->eventFilter(obj, event);
    }

void QRotaryMenu::onEntrySelectionUp() {
    int vec_size = this->entries.size();
    int lower_index = wrapIndex(vec_size, this->selected_entry_index-2-1);
    int upper_index = wrapIndex(vec_size, this->selected_entry_index+2-1);
    int elements_length = std::min(vec_size - std::abs(upper_index - lower_index), std::abs(upper_index - lower_index))+1;
    this->selected_entry->setStyleSheet(this->text);
    this->selected_entry_index = wrapIndex(vec_size, this->selected_entry_index-1);
    this->selected_entry = this->entries[selected_entry_index];
    this->selected_entry->setStyleSheet(this->highlight_text);
    redrawEntries(lower_index, elements_length+1);
}

void QRotaryMenu::onEntrySelectionDown() {
    int vec_size = this->entries.size();
    int lower_index = wrapIndex(vec_size, this->selected_entry_index-2+1);
    int upper_index = wrapIndex(vec_size, this->selected_entry_index+2+1);
    int elements_length = std::min(vec_size - std::abs(upper_index - lower_index), std::abs(upper_index - lower_index))+1;
    this->selected_entry->setStyleSheet(this->text);
    this->selected_entry_index = wrapIndex(vec_size, this->selected_entry_index+1);
    this->selected_entry = this->entries[selected_entry_index];
    redrawEntries(lower_index, elements_length+1);
    this->selected_entry->setStyleSheet(this->highlight_text);
}
