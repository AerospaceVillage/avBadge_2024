#include <QLabel>
#include <QKeyEvent>
#include <cmath>
#include "qradialmenu.h"

QRadialMenu::QRadialMenu(QList<QChar> keyboard_characters, QWidget* parent, QString text, QString highlight_text) {
    this->keyboard_characters = keyboard_characters;
    this->parent = parent;
    this->text = text;
    this->highlight_text = highlight_text;

    QWidget* central_widget = new QWidget(this->parent);
    central_widget->setObjectName(QString::fromUtf8("centralwidget"));
    central_widget->setStyleSheet("background-color:rgb(0);");

    this->text_box = new QLabel(this->parent);
    this->text_box->setText("Flag: ");
    this->text_box->setAlignment(Qt::AlignLeft);
    this->text_box->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");
    this->text_box->setGeometry(10, 10, 500, 20);

    int vec_size = this->keyboard_characters.size();
    double angle_increment = 2 * M_PI / vec_size;
    int radius = 120;
    int font_size_offset = 16/2;
    int x_center = 225-font_size_offset;
    int y_center = 175-font_size_offset;
    for (int i=0; i<vec_size; i++) {
        QLabel* character_label = new QLabel(this->parent);
        character_label->setText(this->keyboard_characters[i]);
        character_label->setAlignment(Qt::AlignLeft);
        character_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        character_label->setStyleSheet("font-size: 16px; color: white; background-color:rgb(0);");

        int x_pos = x_center+(radius*-cos(angle_increment*i));
        int y_pos = y_center+(radius*-sin(angle_increment*i));
        character_label->setGeometry(QRect(x_pos, y_pos, 50, 50));

        character_label->installEventFilter(this);

        this->character_widgets.append(character_label);
    }

    this->selected_entry_index = 0;
    this->selected_entry = this->character_widgets[this->selected_entry_index];
    this->selected_entry->setStyleSheet(this->highlight_text);
    QPoint pos = this->selected_entry->pos();
    this->selected_entry->setGeometry(QRect(pos.x(), pos.y(), 50, 50)); // TODO Offset the correct amount according to the radian.
}

QRadialMenu::~QRadialMenu() {
    for (int i=0; i<this->character_widgets.size(); i++) {
        delete this->character_widgets[i];
    }
}

bool QRadialMenu::eventFilter(QObject *obj, QEvent *event) {
    extern QWidget* primary_control;
    if (this == primary_control) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
                case Qt::Key_Left: {
                    onEntrySelectionLeft();
                    break;
                }
                case Qt::Key_Right: {
                    onEntrySelectionRight();
                    break;
                }
                case Qt::Key_Return: {
                    onEntrySelectionConfirm();
                    break;
                }
                default:
                    break;
            }
        }
        return obj->eventFilter(obj, event);
    }
    return false;
}

void QRadialMenu::onEntrySelectionLeft() {
    this->last_selected_entry_index = this->selected_entry_index;
    this->last_selected_entry = this->character_widgets[this->last_selected_entry_index];
    if (this->selected_entry_index == 0) {
        this->selected_entry_index = this->character_widgets.size()-1;
    }
    else {
        this->selected_entry_index--;
    }

    this->selected_entry = this->character_widgets[this->selected_entry_index];

    this->last_selected_entry->setStyleSheet(this->text);
    this->selected_entry->setStyleSheet(this->highlight_text);
}

void QRadialMenu::onEntrySelectionRight() {
    this->last_selected_entry_index = this->selected_entry_index;
    this->last_selected_entry = this->character_widgets[this->last_selected_entry_index];
    if (this->selected_entry_index == this->character_widgets.size()-1) {
        this->selected_entry_index = 0;
    }
    else {
        this->selected_entry_index++;
    }

    this->selected_entry = this->character_widgets[this->selected_entry_index];

    this->last_selected_entry->setStyleSheet(this->text);
    this->selected_entry->setStyleSheet(this->highlight_text);
}

void QRadialMenu::onEntrySelectionConfirm() {
    this->text_box_string += this->keyboard_characters[this->selected_entry_index];
    this->text_box->setText(this->text_box_string_header + this->text_box_string);
}
