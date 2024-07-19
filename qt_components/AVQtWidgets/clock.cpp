#include "clock.h"
#include "ui_clock.h"
#include <QTime>
#include <QTimer>
#include <QDateTime>

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
}

void Clock::showTime(){
    ui->label->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
//      ui->label->setText(QTime::fromMSecsSinceStartOfDay(10 * 1000).toString("hh:mm:ss"));
}

Clock::~Clock()
{
    delete ui;
}
