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
        QString text="font-size: 16px; color: white; background-color:rgb(0);",
        QString highlight_text="font-size: 24px; color: yellow; background-color:rgb(0);"
        );
    ~QRotaryMenu(); // TODO: Ensure all destructors fire.

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    private slots:
        void onEntrySelectionUp();
        void onEntrySelectionDown();


private:
    int wrapIndex(int size, int index);
    void redrawEntries(int lower_index=0, int element_length=5);
    QList<QRotaryMenuEntry*> entries;
    QString text;
    QString highlight_text;
    QRotaryMenuEntry* selected_entry; // TODO: Make this whatever is the first option / default option.
    int selected_entry_index;
    QWidget* parent;
};



#endif //QROTARYMENU_H