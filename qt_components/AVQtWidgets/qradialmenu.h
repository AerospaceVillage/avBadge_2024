#ifndef QRADIALMENU_H
#define QRADIALMENU_H

#include <QLabel>


class QRadialMenu: public QWidget {
    Q_OBJECT

public:
    explicit QRadialMenu(
        QList<QChar> keyboard_characters,
        QWidget* parent = nullptr,
        QString text="font-size: 16px; color: white; background-color:rgb(0);",
        QString highlight_text="font-size: 24px; color: yellow; background-color:rgb(0);"
        );
    ~QRadialMenu();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    private slots:
    void onEntrySelectionLeft();
    void onEntrySelectionRight();
    void onEntrySelectionConfirm();

private:
    QList<QChar> keyboard_characters;
    QString text;
    QString highlight_text;
    QWidget* parent;

    QList<QWidget*> character_widgets;
    QWidget* selected_entry;
    int selected_entry_index;
    QWidget* last_selected_entry;
    int last_selected_entry_index;
    QLabel* text_box;
    QString text_box_string;
    QString text_box_string_header = "Flag: ";
};



#endif //QRADIALMENU_H
