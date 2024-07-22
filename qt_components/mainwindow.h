#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QEvent>
#include "AVQtWidgets/qrotarymenu.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void set_active_controlled_widget(QWidget* widget);

private:
    Ui::MainWindow *ui;
    QList<QLabel*> labels;
    QRotaryMenu* main_rotary_menu;
};

#endif // MAINWINDOW_H
