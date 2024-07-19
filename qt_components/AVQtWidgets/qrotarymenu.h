#ifndef QROTARYMENU_H
#define QROTARYMENU_H

#include "qrotarymenuentry.h"
#include <QKeyEvent>


class QRotaryMenu: public QWidget {
    Q_OBJECT

public:
    explicit QRotaryMenu(
        QList<QRotaryMenuEntry*> entries,
        QWidget* parent = nullptr,
        int x_offset=50,
        int y_offset=60,
        QString text="font-size: 16px; color: white; background-color:rgb(0);",
        QString highlight_text="font-size: 24px; color: yellow; background-color:rgb(0);"
        );
    ~QRotaryMenu();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    private slots:
        void onEntrySelectionUp();
        void onEntrySelectionDown();
        void onEntrySelectionConfirm();

private:
    int wrapIndex(int size, int index);
    void redrawEntries(int lower_index, int element_length);
    QList<QRotaryMenuEntry*> entries;
    QString text;
    QString highlight_text;
    QRotaryMenuEntry* selected_entry;
    int selected_entry_index;
    QWidget* parent;
    int x_offset;
    int y_offset;
};



#endif //QROTARYMENU_H
