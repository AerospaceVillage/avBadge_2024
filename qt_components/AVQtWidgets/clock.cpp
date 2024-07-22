#include "clock.h"
#include "ui_clock.h"
#include <QTime>
#include <QTimer>
#include <QDateTime>
#include <QKeyEvent>

//QTime* qt;
QDateTime *qdt;
Clock::Clock(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Clock)
{
    QTimer* tir = new QTimer();
//    qt = new QTime();
    qdt = new QDateTime();
    connect(tir, &QTimer::timeout, this, QOverload<>::of(&Clock::showTime));
    tir->start(1000);

    ui->setupUi(this);
    this->parent = parent;
}

void Clock::showTime(){
    ui->label->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
//      ui->label->setText(QTime::fromMSecsSinceStartOfDay(10 * 1000).toString("hh:mm:ss"));
}

Clock::~Clock()
{
    delete ui;
}

bool Clock::eventFilter(QObject *obj, QEvent *event) {
    extern QWidget* primary_control;
    if (this == primary_control) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
                case Qt::Key_Q: {
                    onQPress();
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

void Clock::onQPress() {
    extern QWidget* primary_control;
    primary_control = this->return_to;
    delete this;
}

void Clock::set_return_to(QWidget* return_to) {
    this->return_to = return_to;
}
